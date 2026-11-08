#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("powtty");
    app.setOrganizationName("mume");

    // Load app window icon
    app.setWindowIcon(QIcon(":/resources/icons/icon.png"));

    // Load embedded PowTTY Mono font
    int fontId = QFontDatabase::addApplicationFont(":/resources/fonts/powttymono.ttf");
    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            qDebug() << "Loaded font family:" << fontFamilies.first();
        }
    } else {
        qWarning() << "Failed to load embedded powttymono.ttf font resource.";
    }

    // Modern Dark Theme application stylesheet
    app.setStyleSheet(
        "QMainWindow, QDialog, QWidget { background-color: #1e1e1e; color: #e0e0e0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; font-size: 13px; }"
        "QGroupBox { font-weight: bold; border: 1px solid #3c3c3c; border-radius: 6px; margin-top: 10px; padding-top: 15px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #4ec9b0; }"
        "QLineEdit, QSpinBox, QFontComboBox, QComboBox { background-color: #252526; color: #ffffff; border: 1px solid #3c3c3c; border-radius: 4px; padding: 5px 8px; selection-background-color: #264f78; }"
        "QLineEdit:focus, QSpinBox:focus, QFontComboBox:focus, QComboBox:focus { border: 1px solid #007acc; }"
        "QPushButton { background-color: #0e639c; color: #ffffff; border: none; border-radius: 4px; padding: 6px 14px; font-weight: 500; min-height: 20px; }"
        "QPushButton:hover { background-color: #1177bb; }"
        "QPushButton:pressed { background-color: #0d598a; }"
        "QPushButton:disabled { background-color: #333333; color: #777777; }"
        "QListWidget, QTableWidget { background-color: #252526; color: #ffffff; border: 1px solid #3c3c3c; border-radius: 4px; gridline-color: #333333; }"
        "QListWidget::item:selected, QTableWidget::item:selected { background-color: #094771; color: #ffffff; }"
        "QListWidget::item:hover, QTableWidget::item:hover { background-color: #2a2d2e; }"
        "QHeaderView::section { background-color: #1e1e1e; color: #cccccc; border: 1px solid #333333; padding: 4px; font-weight: bold; }"
        "QTabWidget::pane { border: 1px solid #3c3c3c; background-color: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #969696; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
        "QTabBar::tab:selected { background: #1e1e1e; color: #ffffff; border-top: 2px solid #007acc; }"
        "QTabBar::tab:hover:!selected { background: #383838; }"
        "QCheckBox { spacing: 8px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; border: 1px solid #555555; background: #252526; }"
        "QCheckBox::indicator:checked { background: #007acc; border-color: #007acc; }"
        "QScrollBar:vertical { background: #1e1e1e; width: 12px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #424242; min-height: 20px; border-radius: 6px; margin: 2px; }"
        "QScrollBar::handle:vertical:hover { background: #4f4f4f; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    MainWindow w;
    w.show();

    return app.exec();
}
