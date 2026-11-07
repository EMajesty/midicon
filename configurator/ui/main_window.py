import sys
from ..models import defaults,Action,Behavior
from ..serialization import load_json,save_json
from ..protocol import Device

LABELS=([f"PAD {i+1}" for i in range(16)]+[f"ENC {i+1}" for i in range(4)]+[f"ENC PB {i+1}" for i in range(4)]+[f"FADER {i+1}" for i in range(4)]+["JOY 1 X","JOY 1 Y","JOY 2 X","JOY 2 Y"])

def run():
    try:
        from PySide6.QtWidgets import (QApplication,QFileDialog,QHBoxLayout,QLabel,QLineEdit,QMainWindow,QMessageBox,QPushButton,QSpinBox,QTabWidget,QTableWidget,QTableWidgetItem,QVBoxLayout,QWidget)
    except ImportError: raise SystemExit("Install PySide6 to use the GUI")
    class Window(QMainWindow):
        def __init__(self):
            super().__init__();self.config=defaults();self.tables=[];self.dirty=False
            self.setWindowTitle("MIDIcon Configurator");root=QWidget();layout=QVBoxLayout(root);bar=QHBoxLayout();self.port=QLineEdit("/dev/ttyUSB0");bar.addWidget(QLabel("Port"));bar.addWidget(self.port)
            for text,fn in (("Open",self.open),("Export",self.export),("Download",self.download),("Apply RAM",self.upload),("Save EEPROM",self.save_eeprom),("Reload EEPROM",self.reload)):
                b=QPushButton(text);b.clicked.connect(fn);bar.addWidget(b)
            layout.addLayout(bar);self.tabs=QTabWidget();layout.addWidget(self.tabs);self.status=QLabel("Defaults loaded — not saved to EEPROM");layout.addWidget(self.status);self.setCentralWidget(root);self.populate();self.resize(920,680)
        def populate(self):
            self.tabs.clear();self.tables=[]
            for p,page in enumerate(self.config.pages):
                table=QTableWidget(32,5);table.setHorizontalHeaderLabels(("Control","Action","Behavior","Number","Channel"))
                for r,m in enumerate(page.mappings):
                    for c,v in enumerate((LABELS[r],Action(m.action).name,Behavior(m.behavior).name,str(m.number),str(m.channel))):table.setItem(r,c,QTableWidgetItem(v))
                table.itemChanged.connect(lambda _item,p=p:self.changed(p));self.tables.append(table);self.tabs.addTab(table,page.name)
        def changed(self,p):
            t=self.tables[p]
            try:
                for r,m in enumerate(self.config.pages[p].mappings):m.action=Action[t.item(r,1).text().upper()].value;m.behavior=Behavior[t.item(r,2).text().upper()].value;m.number=int(t.item(r,3).text());m.channel=int(t.item(r,4).text())
                self.dirty=True;self.status.setText("Working configuration changed — EEPROM not saved")
            except (KeyError,ValueError):self.status.setText("Invalid table value")
        def open(self):
            path,_=QFileDialog.getOpenFileName(self,"Open configuration",filter="JSON (*.json)")
            if path:self.config=load_json(path);self.populate();self.dirty=True
        def export(self):
            path,_=QFileDialog.getSaveFileName(self,"Export configuration",filter="JSON (*.json)")
            if path:save_json(self.config,path)
        def device(self):return Device(self.port.text())
        def upload(self):self.device().upload(self.config);self.status.setText("Applied to device RAM — EEPROM not saved")
        def download(self):self.config=self.device().download();self.populate();self.status.setText("Downloaded device working configuration")
        def save_eeprom(self):self.device().save();self.dirty=False;self.status.setText("Device EEPROM saved")
        def reload(self):self.device().reload();self.download()
    app=QApplication(sys.argv);win=Window();win.show();return app.exec()
