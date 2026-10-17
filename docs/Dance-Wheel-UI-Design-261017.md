# Dance Wheel UI Design 261017

本说明替代 `docs/Dance-Wheel-UI-Design-261016.md`。舞蹈轮盘按住 Tab 打开，松开 Tab 关闭。打开后不显示鼠标，按鼠标相对轮盘中心的方向选中扇区。只有在轮盘打开期间按下鼠标左键，才会播放当前选中的舞蹈。表现仍由 Widget Blueprint 负责。

## 开关

- Tab 按下：打开轮盘。已经因为本次按住期间的左键而关闭时，在松开 Tab 之前不要再次打开。
- Tab 松开：关闭轮盘。不播放舞蹈，不播放点击音效。
- 鼠标左键，且当前有选中扇区：播放点击音效，关闭轮盘，调用 `PlayDance(Index)`。此后若 Tab 仍按住，轮盘保持关闭，松开 Tab 时不再调用 `PlayDance`。
- 鼠标左键，且当前没有选中扇区（鼠标在死区内，或还没有任何扇区被选中）：不播放舞蹈，不播放点击音效，轮盘保持打开。
- 打开期间不显示鼠标。输入模式为 Game And UI，游戏不暂停，角色移动可用。调用 `PlayerController::SetIgnoreLookInput(true)`，避免视角吃掉鼠标移动。关闭时 `SetIgnoreLookInput(false)`，回到 Game Only。
- Tab 不在按钮之间移动焦点。轮盘控件及其子控件的四个方向导航都设为 Stop。Tab 的按下和松开由轮盘输入吃掉。

## 选中

- 中心是 `Panel_Sectors` 在视口上的中心，单位是像素。
- 每帧读取鼠标位置。它相对中心的距离小于 `SelectionDeadZone` 时，不改变当前选中。`SelectionDeadZone` 在轮盘控件上，默认 80。
- 距离大于等于死区时，在 `Panel_Sectors` 的按钮里，选与鼠标方向夹角最小的一颗。夹角相同则取收集顺序里更靠前的那颗。
- 收集规则：`Panel_Sectors` 下全部 `Button` 后代，按控件树前序。`PlayDance` 的 `Index` 就是这个顺序里的下标，从 0 开始。增删按钮只改这个面板里的内容。
- 选中从一颗换成另一颗时：对新下标调用 `OnSectorSelected`，对旧下标调用 `OnSectorDeselected`，并播放一次悬停音效。同一颗保持选中时不重复播放。
- 关闭轮盘时，若当时有选中项，对它调用 `OnSectorDeselected`，然后清空选中。
- `OnSectorSelected` 和 `OnSectorDeselected` 是蓝图事件，参数为 `Index`。扇区动画在这两个事件里播放和停止。

悬停音效和点击音效仍是轮盘上的各一份，可空。为空则不播放。`HoverSoundStartTime` 和 `ClickSoundStartTime` 传给 `PlaySound2D` 的 StartTime，默认 0。

## 编辑器控件

父类是 `UDanceWheelWidget`。资源路径 `/Game/UI/DanceWheelUI/WBP_DanceWheel`。代码加载 `/Game/UI/DanceWheelUI/WBP_DanceWheel.WBP_DanceWheel_C`。路径或父类不对时，Tab 仍会绑定，轮盘不出现，并打日志。

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `Panel_Sectors` | Panel Widget（Canvas Panel 或 Overlay 均可） | 扇区按钮的父级。其中心是选中计算的原点 |
| 其下的各个扇区 | Button | 一颗舞蹈。位置由你摆。不使用按钮自身的 Hover 来决定选中 |

`Btn_Dismiss` 和按名字绑定的 `Btn_Dance_0` 到 `Btn_Dance_7` 不再参与逻辑。现有按钮移进 `Panel_Sectors` 即可。找不到 `Panel_Sectors`，或其中没有按钮时，轮盘可以打开，但无法选中，并打日志。

| 属性 | 类型 | 作用 |
| --- | --- | --- |
| `HoverSound` | Sound Base | 选中变化时的音效，可空 |
| `HoverSoundStartTime` | float | 默认 0 |
| `ClickSound` | Sound Base | 左键确认时的音效，可空 |
| `ClickSoundStartTime` | float | 默认 0 |
| `SelectionDeadZone` | float | 相对 `Panel_Sectors` 中心的死区半径，像素，默认 80 |

## 代码结构

- `UDanceWheelSubsystem`：游戏世界开始时生成 `ADanceWheelHost`。专用服务器不生成。
- `ADanceWheelHost`：绑定 Tab 按下和松开，开关轮盘，开关期间忽略视角，恢复输入模式。
- `UDanceWheelWidget`：收集 `Panel_Sectors` 里的按钮，按方向更新选中，处理左键、音效、关闭和 `PlayDance`。

`PlayDance` 仍是 `BlueprintNativeEvent`。默认实现只打日志。

## 不在本次范围

- 每个舞蹈单独的音效或图标数据。
- `PlayDance` 里的真实角色动画。扇区 UI 动画由蓝图事件接到 Widget Animation。
- 圆盘几何体跟着鼠标旋转。
