# Dance Wheel UI Design 261016

已替代。以 `docs/Dance-Wheel-UI-Design-261017.md` 为准。

固定 8 扇区的舞蹈选择轮盘。圆盘不旋转。鼠标悬停选中某一块并播放悬停音效；点击该块播放点击音效、关闭轮盘，并调用舞蹈播放（当前空实现）。表现由编辑器里的 Widget Blueprint 负责，C++ 只绑定约定名称的控件。

## 行为

- Tab 按下：轮盘从关到开，或从开到关。
- 打开：显示鼠标。输入模式为 Game And UI，不暂停游戏，不拦截角色移动和视角。
- 关闭（再按 Tab，或点击轮盘外空白）：隐藏轮盘，回到 Game Only，隐藏鼠标。不播放舞蹈，不播放点击音效。
- 悬停某个舞蹈扇区：播放共用悬停音效。离开后再进入同一扇区会再播一次。空白层不播悬停音效。
- 点击某个舞蹈扇区：播放共用点击音效，关闭轮盘，调用 `PlayDance(Index)`。`Index` 为 0 到 7。`PlayDance` 当前只打日志。
- 8 个扇区共用一份悬停音效、一份点击音效。音效资源可空；为空则不播放。

## 编辑器里要创建的控件

父类必须是 `UDanceWheelWidget`。资源路径：`/Game/UI/DanceWheelUI/WBP_DanceWheel`。代码用 `StaticLoadClass` 加载生成类 `/Game/UI/DanceWheelUI/WBP_DanceWheel.WBP_DanceWheel_C`。路径或父类不对时，Tab 仍会绑定，但轮盘不会出现，并打一条日志。

在该蓝图里创建这些控件（名称区分大小写）：

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `Btn_Dismiss` | Button | 铺满屏幕，放在轮盘下面。点击关闭轮盘且不播舞蹈 |
| `Btn_Dance_0` | Button | 扇区 0 |
| `Btn_Dance_1` | Button | 扇区 1 |
| `Btn_Dance_2` | Button | 扇区 2 |
| `Btn_Dance_3` | Button | 扇区 3 |
| `Btn_Dance_4` | Button | 扇区 4 |
| `Btn_Dance_5` | Button | 扇区 5 |
| `Btn_Dance_6` | Button | 扇区 6 |
| `Btn_Dance_7` | Button | 扇区 7 |

扇区的外形、贴图、Hover 高亮放在对应 Button 的样式里。缺少某个绑定控件时，该扇区不可选，其余扇区仍可用，并打日志。

在这个 Widget Blueprint 的默认值上指定：

| 属性 | 类型 | 作用 |
| --- | --- | --- |
| `HoverSound` | Sound Base | 悬停音效，可空 |
| `HoverSoundStartTime` | float | 悬停音效的 `PlaySound2D` StartTime，默认 0 |
| `ClickSound` | Sound Base | 点击音效，可空 |
| `ClickSoundStartTime` | float | 点击音效的 `PlaySound2D` StartTime，默认 0 |

## 代码结构

- `UDanceWheelSubsystem`：游戏世界开始时生成 `ADanceWheelHost`。专用服务器不生成。
- `ADanceWheelHost`：绑定 Tab，创建并显示或隐藏轮盘，在关闭时恢复输入模式。
- `UDanceWheelWidget`：绑定上表控件，处理悬停、点击、关闭和 `PlayDance`。

`PlayDance` 是 `BlueprintNativeEvent`。以后要接真实舞蹈时，在 `WBP_DanceWheel` 里覆盖它即可，不必改开关逻辑。

## 不在本次范围

- 扇区数量改成数据驱动列表。
- 每个舞蹈单独的音效或图标数据。
- 真实动画播放。
- 圆盘跟随鼠标旋转。
