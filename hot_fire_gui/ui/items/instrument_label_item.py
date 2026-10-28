from PySide6.QtCore import QRectF
from PySide6.QtGui import QBrush, QColor, QFont, QPen
from PySide6.QtWidgets import QGraphicsObject, QGraphicsSimpleTextItem


class InstrumentLabelItem(QGraphicsObject):
    def __init__(self, tag: str, value_text: str, parent=None):
        super().__init__(parent)
        self.tag_item = QGraphicsSimpleTextItem(tag, self)
        self.value_item = QGraphicsSimpleTextItem(value_text, self)

        self.tag_item.setFont(QFont("Arial", 10, QFont.Bold))
        self.value_item.setFont(QFont("Consolas", 11))
        self.tag_item.setBrush(QBrush(QColor(25, 25, 25)))
        self.value_item.setBrush(QBrush(QColor(0, 70, 140)))
        self.value_item.setPos(0, 18)

    def boundingRect(self) -> QRectF:
        r1 = self.tag_item.mapRectToParent(self.tag_item.boundingRect())
        r2 = self.value_item.mapRectToParent(self.value_item.boundingRect())
        return r1.united(r2).adjusted(-4, -4, 4, 4)

    def paint(self, painter, option, widget=None):
        painter.setPen(QPen(QColor(170, 170, 170), 1))
        painter.setBrush(QBrush(QColor(250, 250, 250, 230)))
        painter.drawRoundedRect(self.boundingRect(), 4, 4)

    def set_value(self, text: str):
        self.value_item.setText(text)
        self.prepareGeometryChange()
        self.update()
