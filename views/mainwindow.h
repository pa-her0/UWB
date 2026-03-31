// -------------------------------------------------------------------------------------------------------------------
//
//  File: MainWindow.h
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractSocket>
#include <QLabel>
#include "serialconnection.h"
#include <QTranslator>
#include <QLocale>
#include <QtQuickWidgets/QQuickWidget>
#include "ViewSettingsWidget.h"
namespace Ui {
class MainWindow;
}
extern bool language;
extern bool startCalibration;
extern QString BTN_sendSocketText;
extern QString LE_ip_addressText;
extern QString ip_addressText;
extern int LE_comTexttoInt;
class SerialConnection;
class GraphicsWidget;
class ConnectionWidget;
class ViewSettingsWidget;
class UwbDataVizWidget;
/**
 * The MainWindow class is the RTLS Controller Main Window widget.
 *
 * It is responsible for setting up all the dock widgets inside it, as weel as the central widget.
 * It also handles global shortcuts (Select All) and the Menu bar.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    GraphicsWidget *graphicsWidget();

    ViewSettingsWidget *viewSettingsWidget();

    UwbDataVizWidget *uwbDataVizWidget();

    /**
     * Create a new menu with the window's action
     * @see QMainWindow::createPopupMenu()
     * @return the new menu instance
     */
    virtual QMenu *createPopupMenu();

    /**
     * createPopupMenu adds the windows actions to \a menu.
     * The are the actions to hide or show dock widgets.
     * @param menu the QMenu instance to which the actions should be added
     * @return the menu object
     */
    QMenu *createPopupMenu(QMenu *menu);

    void saveConfigFile(QString filename, QString cfg);
    void loadConfigFile(QString filename);

    bool event(QEvent *event);//语言切换响应事件

    //static bool language;

    QQuickWidget *m_LoadWidget;


public slots:
    void connectionStateChanged(SerialConnection::ConnectionState);
    void saveViewConfigSettings(void);
    void viewSizeChange(QSize);
    void LoadWidgetVisibleChange(bool);
    void LoadWidgetIndexChange(int);

    void SetLanguage_cn();
    void SetLanguage_en();
protected slots:
    void onReady();

    void loadSettings();
    void saveSettings();

    void onAboutAction();
    void onMiniMapView();

    void onAnchorConfigAction();

    void statusBarMessage(QString status);

private:
    Ui::MainWindow *const ui;
    QMenu *_helpMenu;
    QAction *_aboutAction;
    QLabel *_infoLabel;

    QAction *_anchorConfigAction;
    ConnectionWidget *_cWidget;

    bool _showMainToolBar;
    bool _notConnected;

    QTranslator translator;
    QLocale locale;

  //  ViewSettingsWidget  *settingWidget;

};

#endif // MAINWINDOW_H
