import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import Style 1.0

ColumnLayout {
    id: root
    property int selectedMode: 0
    property int pendingMode: -1
    property int phase: 0
    property bool available: true
    property bool connectionBusy: false
    property bool connected: false
    readonly property bool busy: phase !== 0
    signal modeRequested(int mode)
    spacing: 4

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 40
        radius: 11
        color: AmneziaStyle.color.onyxBlack
        RowLayout {
            anchors.fill: parent
            anchors.margins: 3
            spacing: 4
            Repeater {
                model: [qsTr("All traffic"), qsTr("Addresses"), qsTr("Apps")]
                Button {
                    id: segment
                    required property int index
                    required property string modelData
                    objectName: "quickSplitMode" + index
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true
                    readonly property bool selected: root.selectedMode === index
                    readonly property bool pending: root.busy && root.pendingMode === index
                    // A saved choice stays neutral until the VPN is actually connected.
                    readonly property bool activeMode: selected && root.connected && !root.connectionBusy && !root.busy
                    leftPadding: 4
                    rightPadding: 4
                    enabled: root.available && !root.busy && !root.connectionBusy
                    hoverEnabled: true
                    focusPolicy: Qt.StrongFocus
                    text: modelData
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: modelData
                    Accessible.checkable: true
                    Accessible.checked: selected
                    contentItem: Label {
                        text: segment.text
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.NoWrap
                        color: segment.activeMode ? AmneziaStyle.color.midnightBlack : AmneziaStyle.color.paleGray
                    }
                    background: Item {
                        // Animate only the halo so the label remains readable throughout a switch.
                        opacity: root.available ? 1 : 0.45
                        RectangularGlow {
                            id: halo
                            anchors.fill: surface
                            glowRadius: 5
                            spread: 0.12
                            cornerRadius: surface.radius + glowRadius
                            color: AmneziaStyle.color.goldenApricot
                            visible: segment.activeMode || segment.pending
                            opacity: 0.45
                            SequentialAnimation on opacity {
                                running: segment.pending && root.visible
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.25; to: 0.7; duration: 750; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 0.7; to: 0.25; duration: 750; easing.type: Easing.InOutSine }
                                onStopped: halo.opacity = 0.45
                            }
                        }
                        Rectangle {
                            id: surface
                            anchors.fill: parent
                            radius: 8
                            color: segment.activeMode ? AmneziaStyle.color.goldenApricot
                                   : segment.pending ? AmneziaStyle.color.deepBrown
                                   : segment.selected || segment.down ? AmneziaStyle.color.slateGray
                                   : segment.hovered ? AmneziaStyle.color.charcoalGray : "transparent"
                            border.width: segment.activeFocus || segment.selected || segment.pending ? 1 : 0
                            border.color: segment.activeFocus || segment.activeMode || segment.pending
                                          ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
                            Behavior on color { ColorAnimation { duration: 180 } }
                        }
                    }
                    ToolTip.visible: hovered && !selected
                    ToolTip.text: root.connected ? qsTr("Apply and reconnect VPN") : qsTr("Apply to the next connection")
                    onClicked: root.modeRequested(index)
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        // Keep one status line to avoid shifting the main button during reconnection.
        Layout.preferredHeight: 16
        font.pixelSize: 12
        elide: Text.ElideRight
        ToolTip.visible: statusHover.hovered && truncated
        ToolTip.text: text
        HoverHandler { id: statusHover }
        color: root.busy ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
        text: !root.available ? qsTr("The server controls split tunneling")
              : root.phase === 1 ? qsTr("Disconnecting to apply the mode…")
              : root.phase === 2 ? qsTr("Applying the mode…")
              : root.phase === 3 ? qsTr("Reconnecting VPN…")
              : root.selectedMode === -1 ? qsTr("Both types are enabled. Select one mode.")
              : root.connected ? qsTr("Changing the mode reconnects VPN")
              : qsTr("Applies to the next connection")
    }
}
