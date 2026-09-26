import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
                model: [qsTr("Off"), qsTr("Addresses"), qsTr("Apps")]
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
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        color: segment.selected ? AmneziaStyle.color.midnightBlack : AmneziaStyle.color.paleGray
                    }
                    background: Rectangle {
                        radius: 8
                        color: segment.selected ? AmneziaStyle.color.goldenApricot
                               : segment.down ? AmneziaStyle.color.charcoalGray
                               : segment.hovered ? AmneziaStyle.color.slateGray : "transparent"
                        border.width: segment.activeFocus || segment.pending ? 1 : 0
                        border.color: AmneziaStyle.color.goldenApricot
                        // Retain the selected state while temporarily disabling input.
                        opacity: root.available ? 1 : 0.45
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
