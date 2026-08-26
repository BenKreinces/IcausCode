from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QPen
from PySide6.QtWidgets import QGraphicsLineItem


class PipeItem(QGraphicsLineItem):
    def __init__(self, x1, y1, x2, y2, parent=None):
        super().__init__(x1, y1, x2, y2, parent)
        pen = QPen(QColor(40, 40, 40), 3)
        pen.setCapStyle(Qt.RoundCap)
        self.setPen(pen)
