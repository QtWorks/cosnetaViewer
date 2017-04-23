import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import "."

ComboBox {
    height: Theme.standardWidgetHeight
    style: ComboBoxStyle {
        label: Text {
            font.family: Theme.mainFontFamily
            font.pixelSize: Theme.mainFontSize
            text: control.currentText
            color: Theme.mainFontColor
            textFormat: Text.RichText
            height: Theme.standardWidgetHeight
        }
        property Component __dropDownStyle: MenuStyle {

            __maxPopupHeight: 600
            __menuItemType: "comboboxitem"

            // an item text
            itemDelegate.label: Text {
                font.family: Theme.mainFontFamily
                font.pixelSize: Theme.mainFontSize
                text: styleData.text
                color: Theme.mainFontColor
                textFormat: Text.RichText
                height: Theme.standardWidgetHeight
            }
        }
    }
}
