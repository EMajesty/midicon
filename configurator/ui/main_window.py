import sys
from ..models import defaults,Action,Behavior
def run():
    try:from PySide6.QtWidgets import QApplication,QComboBox,QFormLayout,QGridLayout,QLineEdit,QMainWindow,QSpinBox,QTabWidget,QWidget
    except ImportError:raise SystemExit("Install PySide6 to use the GUI")
    app=QApplication(sys.argv);c=defaults();tabs=QTabWidget()
    for page in c.pages:
        w=QWidget();grid=QGridLayout(w);name=QLineEdit(page.name);grid.addWidget(name,0,0,1,4)
        for i in range(16):
            box=QComboBox();box.addItems([x.name for x in Action]);box.setCurrentIndex(page.mappings[i].action);grid.addWidget(box,1+i//4,i%4)
        tabs.addTab(w,page.name)
    win=QMainWindow();win.setWindowTitle("MIDIcon Configurator");win.setCentralWidget(tabs);win.resize(760,520);win.show();return app.exec()
