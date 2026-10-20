# Dance Wheel UI Design 261017

本说明替代 `docs/Dance-Wheel-UI-Design-261016.md`。舞蹈轮盘按住 Tab 打开，松开 Tab 关闭。打开后显示鼠标，按鼠标相对轮盘中心的方向选中扇区。只有在轮盘打开期间按下鼠标左键，才会播放当前选中的舞蹈。表现仍由 Widget Blueprint 负责。

## 开关

- Tab 按下：打开轮盘。已经因为本次按住期间的左键而关闭时，在松开 Tab 之前不要再次打开。
- Tab 松开：关闭轮盘。不播放舞蹈，不播放点击音效。
- 鼠标左键，且当前有选中扇区：播放点击音效，关闭轮盘，调用 `PlayDance(Index)`。此后若 Tab 仍按住，轮盘保持关闭，松开 Tab 时不再调用 `PlayDance`。
- 鼠标左键，且当前没有选中扇区（鼠标在死区内，或还没有任何扇区被选中）：不播放舞蹈，不播放点击音效，轮盘保持打开。
- 打开期间是否显示鼠标由 `bShowCursorWhileOpen` 决定，默认显示。输入模式为 Game And UI，游戏不暂停，角色移动可用。调用 `PlayerController::SetIgnoreLookInput(true)`，避免视角吃掉鼠标移动。关闭时 `SetIgnoreLookInput(false)`，回到 Game Only，并隐藏鼠标。
- Tab 不在按钮之间移动焦点。轮盘控件及其子控件的四个方向导航都设为 Stop。Tab 的按下和松开由轮盘输入吃掉。

## 选中

- 中心是 `Panel_Sectors` 在视口上的中心，单位是像素。
- 鼠标被限制在以 `Panel_Sectors` 中心为圆心的圆环内。`CursorInnerRadius` 默认 80，`CursorOuterRadius` 默认 280，单位像素。更近则推到内径，更远则拉到外径，方向不变。位于正中心时沿正上方推到内径。外径小于内径时按内径处理。
- 在圆环上按等分扇区选中。0° 是正上方，角度沿顺时针增加。第 0 颗的中心角是 `FirstSectorAngle`，之后每颗加 `360/N` 度。每颗占自己中心角左右各 `180/N` 度。正好落在分界线上时归入下一颗。
- 扇区由代码在 `Panel_Sectors` 里按 `WBP_DanceSector` 生成，不再手摆。`SectorCount` 是个数，默认 8。`SectorRadius` 是生成半径，像素，默认 200。`FirstSectorAngle` 是第 0 颗相对正上方的顺时针偏移，度，默认 0。生成时会清掉面板里原有的子控件。`Index` 按生成顺序从 0 开始。
- 选中从一颗换成另一颗时：旧实例调用 `OnSectorUnhovered`，新实例调用 `OnSectorHovered`，并播放一次悬停音效。同一颗保持选中时不重复调用。轮盘上的 `OnSectorDeselected` / `OnSectorSelected` 仍会带上下标调用。
- 从没有选中变成某一颗时，只调用那颗的 `OnSectorHovered`。
- 内径上仍会按方向选中。只有鼠标在正中心、内径又是 0 时不改变选中。
- 每次打开轮盘时，对每一颗扇区先调用 `ResetToDefault`，再调用 `OnWheelUIOpened`。`ResetToDefault` 把 Widget Animation 拨回第 0 帧并停止。`OnWheelUIOpened` 是扇区上的蓝图事件，供按钮刷新颜色。关闭时仍然不调用 `OnSectorUnhovered`。
- `OnSectorHovered` 和 `OnSectorUnhovered` 是 `UDanceSectorWidget` 上的 `BlueprintNativeEvent`。C++ 默认实现为空。不使用 `Button` 的 `OnHovered`。扇区动画在蓝图里重写这两个事件来播放。

悬停音效和点击音效仍是轮盘上的各一份，可空。为空则不播放。`HoverSoundStartTime` 和 `ClickSoundStartTime` 传给 `PlaySound2D` 的 StartTime，默认 0。

## 编辑器控件

父类是 `UDanceWheelWidget`。资源路径 `/Game/UI/DanceWheelUI/WBP_DanceWheel`。代码加载 `/Game/UI/DanceWheelUI/WBP_DanceWheel.WBP_DanceWheel_C`。路径或父类不对时，Tab 仍会绑定，轮盘不出现，并打日志。

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `Panel_Sectors` | Canvas Panel | 扇区的父级，必须是 Canvas Panel。其中心是选中和生成的原点。里面不要再手摆扇区 |

代码按 `/Game/UI/DanceWheelUI/WBP_DanceSector` 生成扇区。找不到 `Panel_Sectors` 或找不到这个蓝图时，轮盘可以打开，但无法选中，并打日志。

| 属性 | 类型 | 作用 |
| --- | --- | --- |
| `HoverSound` | Sound Base | 选中变化时的音效，可空 |
| `HoverSoundStartTime` | float | 默认 0 |
| `ClickSound` | Sound Base | 左键确认时的音效，可空 |
| `ClickSoundStartTime` | float | 默认 0 |
| `CursorInnerRadius` | float | 鼠标圆环内径，像素，默认 80 |
| `CursorOuterRadius` | float | 鼠标圆环外径，像素，默认 280 |
| `bShowCursorWhileOpen` | bool | 轮盘打开时是否显示鼠标，默认勾选 |
| `SectorCount` | int | 生成的扇区个数，默认 8 |
| `SectorRadius` | float | 扇区中心到 `Panel_Sectors` 中心的距离，像素，默认 200 |
| `SectorScale` | float | 扇区里名为 `DanceSectorButton` 的 Button 的 Render Transform Scale，默认 1 |
| `FirstSectorAngle` | float | 第 0 颗相对正上方的顺时针角度，度，默认 0 |

## 代码结构

- `UDanceWheelSubsystem`：游戏世界开始时生成 `ADanceWheelHost`。专用服务器不生成。
- `ADanceWheelHost`：绑定 Tab 按下和松开，开关轮盘，开关期间忽略视角，恢复输入模式。
- `UDanceWheelWidget`：收集 `Panel_Sectors` 里的 `UDanceSectorWidget`，按方向更新选中。换成另一颗时调用旧扇区的 `OnSectorUnhovered` 和新扇区的 `OnSectorHovered`。处理左键、音效、关闭和 `PlayDance`。
- `UDanceSectorWidget`：一颗扇区。`OnSectorHovered` 与 `OnSectorUnhovered` 供蓝图重写，C++ 不播放动画。

`PlayDance` 仍是 `BlueprintNativeEvent`。默认实现只打日志。

## 不在本次范围

- 每个舞蹈单独的音效或图标数据。
- `PlayDance` 里的真实角色动画。扇区 UI 动画由蓝图重写 `OnSectorHovered` / `OnSectorUnhovered` 播放。
- 圆盘几何体跟着鼠标旋转。
