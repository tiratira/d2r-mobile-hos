import struct,sys,os,glob
# 通用版:SPIR-V 能力清洗。Huawei Maleoon 驱动对「声明了但未使用的不支持能力」
# 会返回 VK_ERROR_OUT_OF_HOST_MEMORY(实测 Float64/ImageBuffer/ImageMSArray/
# DerivativeControl/GroupNonUniform* 均触发)。按「对应类型/指令是否存在」删除。
def scrub(path):
    d=open(path,"rb").read()
    if len(d)<20 or d[:4]!=b"\x03\x02\x23\x07": return None
    n=len(d)//4; w=list(struct.unpack_from("<%dI"%n,d,0))
    i=5; ins=[]
    while i<n:
        wc=w[i]>>16; op=w[i]&0xffff
        ins.append((i,wc,op)); i+=wc
    def has_float(width): return any(op==22 and w[j+1]==width for j,wc,op in ins if wc>=3)
    def has_int(width): return any(op==21 and w[j+1]==width for j,wc,op in ins if wc>=4)
    def has_img(dim): return any(op==25 and wc>=9 and w[j+2]==dim for j,wc,op in ins)
    def has_any(ops): return any(op in ops for j,wc,op in ins)
    caps={w[j+1] for j,wc,op in ins if op==17 and wc>=2}
    strip=set()
    if 10 in caps and not has_float(64): strip.add(10)
    if 9 in caps and not has_float(16): strip.add(9)
    if 11 in caps and not has_int(64): strip.add(11)
    if 22 in caps and not has_int(16): strip.add(22)
    if 38 in caps and not has_int(8): strip.add(38)
    if 46 in caps and not has_img(5): strip.add(46)
    if 47 in caps and not any(w[j+6]==1 for j,wc,op in ins if op==25 and wc>=9): strip.add(47)
    if 50 in caps and not has_any({207,208,209,210}): strip.add(50)
    if 25 in caps and not has_any({83,84,86,87,88,89,90,91}): strip.add(25)
    if 62 in caps or 64 in caps or 65 in caps:
        if not has_any({338,339,340,341,342,343,344,345,346,347,348,349,350}):
            strip.update({62,64,65})
    if not strip: return None
    i=5; out=[]
    while i<n:
        wc=w[i]>>16; op=w[i]&0xffff
        if not (op==17 and wc>=2 and w[i+1] in strip): out.append((i,wc))
        i+=wc
    nw=w[:5]
    for j,wc in out: nw.extend(w[j:j+wc])
    return struct.pack("<%dI"%len(nw),*nw), sorted(strip)
if __name__=="__main__":
    src,out=sys.argv[1],sys.argv[2]
    os.makedirs(out,exist_ok=True)
    tot=chg=0; caps={}
    for f in glob.glob(os.path.join(src,"*.spv")):
        r=scrub(f); tot+=1
        b=os.path.basename(f)
        if r: open(os.path.join(out,b),"wb").write(r[0]); chg+=1
        else: open(os.path.join(out,b),"wb").write(open(f,"rb").read())
        for c in (r[1] if r else []): caps[c]=caps.get(c,0)+1
    print(f"scrub {tot} spv, changed {chg}, stripped caps {caps}")
