from PySide6.QtCore import QRectF, Qt
from PySide6.QtGui import QColor, QFont, QPainter, QPen
from PySide6.QtWidgets import QGraphicsObject, QGraphicsSimpleTextItem


class PumpItem(QGraphicsObject):
    def __init__(self, label: str = "PUMP", point_up: bool = True, parent=None):
        super().__init__(parent)
        self.label_item = QGraphicsSimpleTextItem(label, self)
        self.label_item.setFont(QFont("Arial", 10, QFont.Bold))
        self.label_item.setPos(-22, 34)
        self.point_up = point_up

    def boundingRect(self) -> QRectF:
        return QRectF(-38, -34, 76, 84)

    def paint(self, painter: QPainter, option, widget=None):
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setPen(QPen(QColor(20, 20, 20), 2.5))
        painter.setBrush(Qt.NoBrush)
        painter.drawEllipse(QRectF(-22, -22, 44, 44))
        painter.save()
        painter.rotate(-90 if self.point_up else 90)
        painter.drawLine(-14, 16, 22, 0)
        painter.drawLine(22, 0, -14, -16)
        painter.restore()
