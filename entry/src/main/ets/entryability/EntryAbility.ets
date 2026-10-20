/*
 * EntryAbility — D2R Mobile HOS 入口。
 * 全屏沉浸(隐藏系统栏)、屏幕常亮、把应用沙箱 files 目录写入
 * AppStorage 供 native 宿主使用。
 */
import { AbilityConstant, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';

const DOMAIN = 0x0000;

export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    try {
      this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
    } catch (err) {
      hilog.error(DOMAIN, 'D2RMobileHOS', 'Failed to set colorMode. Cause: %{public}s', JSON.stringify(err));
    }
    AppStorage.setOrCreate('filesDir', this.context.filesDir);
    hilog.info(DOMAIN, 'D2RMobileHOS', 'Ability onCreate, filesDir=%{public}s', this.context.filesDir);
  }

  onDestroy(): void {
    hilog.info(DOMAIN, 'D2RMobileHOS', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    hilog.info(DOMAIN, 'D2RMobileHOS', 'Ability onWindowStageCreate');

    windowStage.loadContent('pages/Index', (err) => {
      if (err.code) {
        hilog.error(DOMAIN, 'D2RMobileHOS', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err));
        return;
      }
      // 全屏沉浸:隐藏状态栏与导航栏,游戏铺满屏幕(对应上游 edge-to-edge)。
      try {
        const win = windowStage.getMainWindowSync();
        win.setWindowSystemBarEnable([]);
        win.setWindowKeepScreenOn(true);
        win.setPreferredOrientation(window.Orientation.AUTO_ROTATION_LANDSCAPE);
      } catch (err) {
        hilog.warn(DOMAIN, 'D2RMobileHOS', 'immersive mode failed: %{public}s', JSON.stringify(err));
      }
    });
  }

  onWindowStageDestroy(): void {
    hilog.info(DOMAIN, 'D2RMobileHOS', 'Ability onWindowStageDestroy');
  }

  onForeground(): void {
    hilog.info(DOMAIN, 'D2RMobileHOS', 'Ability onForeground');
  }

  onBackground(): void {
    hilog.info(DOMAIN, 'D2RMobileHOS', 'Ability onBackground');
  }
}
