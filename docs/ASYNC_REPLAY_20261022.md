# 2026-10-22 回放层异步化(提交环 + 动态偏移描述符)

设备:HUAWEI Mate 80 / Maleoon 920。对照上游 iOS(锁定 60fps:全异步 Metal、
离线 metallib、pso 预热)与上游 Android(fence 异步 + 完成线程 + worker 重叠编码
之后才接近 60)。鸿蒙版此前为每帧 2–3 次 `vkQueueWaitIdle` 的全同步回放,
营地实测 7 fps(CPU 回放 84ms + GPU 等待 36ms + 回写 5ms 串行)。

## 架构(entry/src/main/cpp/gfx/dm_vk_replay.cpp)

- **提交环(RING=4)**:每槽独享命令池/命令缓冲/fence/呈现信号量/暂存 arena。
  `vkQueueSubmit` 携带槽 fence 立即返回;游戏线程继续录制下一批。
- **完成处理**:fence 触发后 `slotProcess` 统一执行——客体事件与计数器写入、
  存储缓冲回写(`nvn_dirty_prepare_write` + memcpy)、延迟资源销毁(被替换纹理、
  退休常驻缓冲、帧缓冲)、退休描述符集合释放、命令池复位、arena 回卷。
  完成线程 2ms 轮询 fence,保证游戏在两次提交之间轮询客体事件也能推进。
- **客体同步语义**:`serial` 改为已提交计数;`mtl_wait(s)` 等待完成序号 ≥ s
  (先等最旧在飞槽的 fence,再应用其完成效果),围栏放行保证回写已落地。
  CPU 写客体内存(OP_UPDATE_UB / OP_COPY_B2B / 内建 compute 读写)与挂起回写
  范围重叠时触发 `drainIfOverlap` 全排水。
- **暂存三级**:
  1. 槽 arena(每槽回卷):顶点/索引等一次性数据;
  2. 持久 arena(只增不减,内容寻址):零页、支持页、每着色器常量 bank;
  3. **动态环形缓冲(单块 512MB)**:所有 UBO/SSBO 绑定数据。描述符集合指向
     环形缓冲本身,绘制时以动态偏移定位——数据每帧变化不再使缓存集合失效。
     环回卷水位 = 仍在飞槽位的起始标记的最小值。
- **描述符集合缓存**:按(着色器, 绑定身份)缓存。身份包含缓冲区句柄+偏移+范围、
  纹理序列号+代数+视图哈希、采样器代数;动态绑定只含声明范围。失效源:
  常驻缓冲退休、纹理替换、采样器重注册、槽 arena 回卷(静态回退绑定)。
  失效即时从哈希桶移除,墓碑过半即压实——否则帧率随会话时长单调劣化。
- **管线缓存**:`VkPipelineCache` 落盘 `dm_pipeline_cache.bin`(stats 时保存),
  二次启动免编译。
- **bank 缓存**:驱动常量 bank(slot 0/2)按内容寻址持久化,绑定表不变时
  集合身份跨帧稳定。

## 调试开关(debugFlags,`readFlags` 于每次 begin 读取)

| 位 | 含义 |
|---|---|
| 8 | 禁用常驻快照(residency) |
| 32 | 禁用描述符集合缓存(每次重建) |
| 64 | 禁用 bank 缓存 |
| 16 | 每次提交后全同步(reclaimAll) |
| 128 | present 旧同步语义(blit 完成后才 present + waitIdle) |

编译默认 0(全开)。文件通道:`/data/local/tmp/dm_render_flags.txt`(shell 可写,
注意应用沙箱可能读不到)与应用沙箱 `files/dm_render_flags.txt`。

## 本轮修复的缺陷(按发现顺序,均为真实代价)

1. 槽 arena 首页索引越界(`pages[1]` 于空数组)→ 加载期 SIGSEGV。
2. `copies` 快照跨提交槽复用,槽回卷后内存被改写 → device lost @submit 8。
   修复:每次 finish 清空 `copies`(跨提交复用仅走常驻/持久路径)。
3. `finish()` 中 `if (!check(vkQueueSubmit(...)))` 极性反了:成功时未置
   `live`/未轮转槽位 → 全部提交复用槽 0、fence 从不复位、命令缓冲在飞时被重录
   → device lost @submit 8;完成机制整体空转。同类极性错误在
   `vkCreatePipelineLayout` 的失败缓存路径又犯一次(成功被当作失败,管线全空)。
4. 动态描述符配额按"每集合"用满设备上限,但 `maxDescriptorSet*Dynamic` 是
   整个管线布局(vs+ps 之和)上限 → 布局创建全败 → draws=0。配额减半。
5. 描述符池缺 `*_DYNAMIC` 两类配额 → 含动态绑定的集合分配必败。补齐 10 类。
6. 失效集合墓碑只标记不摘除,桶扫描与全表失效扫描随时长线性增长
   → 帧率从开局 60 单调下滑。撤销时物理摘除 + 过半压实。

## 验证命令

```sh
.venv/bin/python tools/test_replay_cache.py   # copies/dynCopies 失效逻辑(含新表)
.venv/bin/python tools/test_patch_nso_hos.py
devecocli build && devecocli run --skip-build
# 真机进营地后间隔采样:
hdc shell cat .../files/dm_replay_stats.txt   # present 差值 / 墙钟 = 帧率
```

## 已知后续(未做)

- GPU 侧:每帧约 60ms 提交-完成延迟(队列 2 帧深时折算),render pass 未合并、
  `imageBarrier` 全命令域全子资源屏障偏重——对标上游 `tbdr_pass_merge`。
- 呈现节奏器(对标 `nvn_pace`)与 pso 预热清单(对标 `pso_manifest.tsv`)。
- 帧率随场景的内容依赖:写字板(走路闪一下)未定位,已排除着色器/管线编译
  (计数零增长);候选:回写时序边角、纹理替换在飞、环回卷边角。
