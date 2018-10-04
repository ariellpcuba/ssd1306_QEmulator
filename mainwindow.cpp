#include <QtGlobal>

#include <QApplication>
#include <QPushButton>
#include <QTimer>


//#include <QtTest/QtTest>
#include <QDebug>



#include "app_types.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "emulator_ssd1306.h"



/* Include font files here */
#include "font/FreeSans12pt7b.h"
#include "font/FreeSans9pt7b.h"
#include "font/mina.h"


#include <Adafruit_GFX_Menu.h>

#include <Adafruit_Keyboard.h>
//#include "emulator_keypad.h"

uint8_t buffer[128 * 64 / 8];

Bitmap lcd_bitmap;


/* Global emulator instance */
MainWindow         *emul_win;

Adafruit_SSD1306   *display;

Adafruit_GFX_Menu  *emul_menu;  /* Global menu instance */
Adafruit_Keyboard  *emul_kbd;   /* Global keyboard instance */


static char tempStr[32] = {0};
















static void gps_basis_info_display(void)
{
  display->setFont(&mina10pt8b);
  display->setCursor(0, 7);

  sprintf(tempStr, "%s", "[ GPS ]:");
  //qDebug("GPS Menu");
  display->print(tempStr);
}

static void aprs_basis_info_display(void)
{
  display->setFont(&mina10pt8b);
  display->setCursor(0, 7);

  sprintf(tempStr, "%s", "[ APRS ]:");
  //qDebug("APRS Menu");
  display->print(tempStr);
}




uint8_t gps_status = 0;

static void demo1_menu_info_display(void)
{

}

static void gps_status_toggle(iptr_t value)
{
  gps_status ^= ( 1 << *(uint8_t*)&value);
}




static struct MenuItem sys_items[] =
{
  /* Label */                         /* Flags */                   /* Hook */                                  /* User data*/
  { (const_iptr_t)"Active SENSORS",   0,                            (MenuHook)&Adafruit_GFX_Menu::menu_handle,  (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"Active MODULES",   0,                            (MenuHook)&Adafruit_GFX_Menu::menu_handle,  (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"Show VERSION",     MIF_TOGGLE | MIF_SHOWSCREEN,  (MenuHook)demo1_menu_info_display,          (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"ABOUT ...",        MIF_TOGGLE | MIF_SHOWSCREEN,  (MenuHook)demo1_menu_info_display,          (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"Return",           MIF_DISABLED,                 NULL,                                       (iptr_t)0 },
  { (const_iptr_t)0,                  0,                            NULL,                                       (iptr_t)0 }
};
static struct Menu sys_menu = 
{ 
  /* Items */ /* Title */       /* Flags */             /* Bitmap */    /* Selected */  /* lcd_blitBitmap */
  sys_items,  "[ SYS ]: MENU",  MF_STICKY | 0,          &lcd_bitmap,    0,

#if CPU_AVR
  0 };
#else
  0 };
#endif



static struct MenuItem gps_items[] =
{
  /* Label */                         /* Flags */                               /* Hook */                          /* User data*/
  { (const_iptr_t)"Enable GPS",       MIF_CHECKIT | MIF_TOGGLE | MIF_CHECKED,   (MenuHook)gps_status_toggle,        (iptr_t)0 },
  { (const_iptr_t)"Use GPS TIME",     MIF_CHECKIT | MIF_TOGGLE | MIF_CHECKED,   (MenuHook)demo1_menu_info_display,  (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"Set UNITS SI/US",  MIF_CHECKIT | MIF_TOGGLE | MIF_CHECKED,   (MenuHook)demo1_menu_info_display,  (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"Set TIMEZONE",     0,                                        (MenuHook)demo1_menu_info_display,  (iptr_t)&lcd_bitmap },
  { (const_iptr_t)"Return",           MIF_DISABLED,                             NULL,                               (iptr_t)0 },
  { (const_iptr_t)0,                  0,                                        NULL,                               (iptr_t)0 }
};
static struct Menu gps_menu = 
{ 
  /* Items */ /* Title */       /* Flags */             /* Bitmap */    /* Selected */  /* lcd_blitBitmap */
  gps_items,  "[ GPS ]: MENU",  MF_STICKY | 0,          &lcd_bitmap,    0,

#if CPU_HARVARD
  0 };
#else
  0 };
#endif


/* TOP Level Screens to display*/
static struct MenuItem info_items[] =
{
  /* Label */                               /* Flags */     /* Hook */                                  /* User data*/
  { (const_iptr_t)gps_basis_info_display,   MIF_RENDERHOOK, (MenuHook)&Adafruit_GFX_Menu::menu_handle,  (iptr_t)&gps_menu },
  { (const_iptr_t)aprs_basis_info_display,  MIF_RENDERHOOK, (MenuHook)&Adafruit_GFX_Menu::menu_handle,  (iptr_t)&sys_menu },
  { (const_iptr_t)0,                        0,              NULL,                                       (iptr_t)0 }
};
static struct Menu info_screens =
{
  /* Items */ /* Title */       /* Flags */             /* Bitmap */   /* Selected */  /* lcd_blitBitmap */
  info_items, "ROOT",           MF_STICKY | MF_SAVESEL, &lcd_bitmap,   0,              

#if CPU_HARVARD
  0 };
#else
  0 };
#endif











MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    /* Build GUI window configured with Qt Creator in file mainwindow.ui */
    ui->setupUi(this);

    /* Set global pointer to this instance */
    emul_win = this;

    /* Create layout for emulator widget */
    QVBoxLayout* verticalLayout = new QVBoxLayout;
    verticalLayout->setSpacing(0);
    verticalLayout->setMargin(0);
    ui->emulatorContainer->setLayout(verticalLayout);

    /* Create Emulator Widget */
    Adafruit_SSD1306 *emulator = new Adafruit_SSD1306;

    /* Set global display pointer to this instance */
    display = emulator;

    /* Set some default parameter */
    emulator->setEmulatorZoom(2);           /* Set zooming level */
    emulator->setSpeedEmulation(false);     /* Set simulation speed */

    /* Connect Emulator to centralWidget */
    ui->emulatorContainer->layout()->addWidget(emulator);

    /* Create Window Title and Icon */
    this->setWindowTitle("Adafruit OLED emulator ("
            +QString::number(emulator->device_width()  * emulator->getEmulatorZoom()) + "x"
            +QString::number(emulator->device_height() * emulator->getEmulatorZoom()) + ")");

    this->setWindowIcon(QIcon(":/res/icon"));
    
    /* Initialize Emulator */
    int oled_type = OLED_ADAFRUIT_I2C_128x64;
    if (emulator->oled_is_spi_proto(oled_type)){
        //SPI change parameters to fit to your LCD
        if ( !emulator->init(OLED_SPI_DC,OLED_SPI_RESET,OLED_SPI_CS, oled_type) )
            exit(EXIT_FAILURE);
    }
    else{
        //I2C change parameters to fit to your LCD
        if ( !emulator->init(OLED_I2C_RESET, oled_type) )
            exit(EXIT_FAILURE);
    }
    
    emulator->begin();
    

    /* Get size of keypad so we can determine total height of window */
    QMargins keypad_margins = ui->gridLayout->contentsMargins();
    int keypad_height = ui->gridLayout->sizeHint().height() + 
                        keypad_margins.top() +
                        keypad_margins.bottom();

    //qDebug("%d", margins.top());

    /* Resize Window */
    this->setFixedSize(emulator->device_width()  * emulator->getEmulatorZoom(), 
                       emulator->device_height() * emulator->getEmulatorZoom() + 
                       this->menuBar()->size().height() +
                       this->statusBar()->size().height() +  keypad_height );

    this->statusBar()->setSizeGripEnabled(false);

    this->statusBar()->showMessage("Loaded SSD1603..");


    /* Search all buttons and add to a group */
    QButtonGroup* group = new QButtonGroup(this);

    for (int i = 0; i < ui->gridLayout->count(); ++i)
    {
      QWidget *widget = ui->gridLayout->itemAt(i)->widget();
      if (widget != NULL)
      {
        group->addButton(dynamic_cast<QPushButton*>(widget));
      }
    }

    /* Create SLOT for every released button */
    connect(group, SIGNAL(buttonReleased(int)), this, SLOT(onGroupButtonClicked(int)));




    /* Create Menu */
    emul_menu = new Adafruit_GFX_Menu;
    emul_menu->menu_init(&lcd_bitmap, buffer, SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT);

    /* Connect Keyboard */
    emul_kbd  = new Adafruit_Keyboard;
    emul_kbd->kbd_init();

    /* Keyboard timer to poll buttons */
    timerId = startTimer(10);



    /* ==> INSERT CODE HERE - BEGIN <== */

#if 1
    /* Initialize Screen */
    emulator->clearDisplay();   // clears the screen and buffer

    // Set default color and size
    emulator->setTextSize(1);
    emulator->setTextColor(WHITE);

    // Using Adafruit fonts
    emulator->setFont(&FreeSans12pt7b);
    emulator->setCursor(7,31);
    static char tmp[36] = {0};
    sprintf(tmp, "%s", "Hallo Welt!");
    emulator->print(tmp);
    
    // Using glcdfont
    emulator->setFont(&mina10pt8b);
    emulator->setCursor(0, 10);
    sprintf(tmp, "%s", "All your base are us!");
    emulator->print(tmp);
    
    // Use of standard text
    //emulator->setFont(NULL);
    //emulator->print("All your base are us");
#endif

#if 0
    emulator->drawPixel(0, 0, WHITE);
    emulator->drawPixel(127, 0, WHITE);
    emulator->drawPixel(0, 63, WHITE);
    emulator->drawPixel(127, 63, WHITE);
#endif
    /* Must be called at the end to refresh the display */
    emulator->display();

    /* ==> INSERT CODE HERE - END <== */


}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::loop_menu()
{
    emul_menu->screen_handle(&info_screens);
}







void MainWindow::timerEvent(QTimerEvent *event)
{
    Adafruit_Keyboard *poll_kbd = new Adafruit_Keyboard();

    poll_kbd->kbd_poll();
}



int MainWindow::readButtons(void)
{
    /* Search all buttons and add to a group */
    QPushButton* button = new QPushButton(this);

    QString objName = NULL;

    int keymask = 0;

    for (int i = 0; i < ui->gridLayout->count(); ++i)
    {
      QWidget *widget = ui->gridLayout->itemAt(i)->widget();
      if (widget != NULL)
      {
        button = dynamic_cast<QPushButton*>(widget);
        if (button->isDown()) {
            objName = widget->objectName();

            if       (objName == "pushButton_UP")       keymask |= (K_UP);
            else if  (objName == "pushButton_DOWN")     keymask |= (K_DOWN);
            else if  (objName == "pushButton_LEFT")     keymask |= (K_LEFT);
            else if  (objName == "pushButton_RIGHT")    keymask |= (K_RIGHT);
            else if  (objName == "pushButton_OK")       keymask |= (K_OK);

            //qDebug("%s", qPrintable(objName));
        }
      }
    }

    return keymask;
}




#if 0
extern "C" int emul_kbdReadCols(void)
{
    return 0;
}
#endif


















/* SLOT Functions for GUI interaction */

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg(this);
    dlg.exec();
}


/* SLOT Functions for BUTTON interactions */
#define DISABLE_SLOT_MSG    0

void MainWindow::on_pushButton_LEFT_pressed()
{
    ui->statusBar->showMessage("Press LEFT");
    #if DISABLE_SLOT_MSG
    qDebug("LEFT");
    #endif
}

void MainWindow::on_pushButton_RIGHT_pressed()
{
    ui->statusBar->showMessage("Press RIGHT");
    #if DISABLE_SLOT_MSG
    qDebug("RIGHT");
    #endif
}

void MainWindow::on_pushButton_UP_pressed()
{
    ui->statusBar->showMessage("Press UP");
    #if DISABLE_SLOT_MSG
    qDebug("UP");
    #endif
}

void MainWindow::on_pushButton_DOWN_pressed()
{
    ui->statusBar->showMessage("Press DOWN");
    #if DISABLE_SLOT_MSG
    qDebug("DOWN");
    #endif
}

void MainWindow::on_pushButton_OK_pressed()
{
    ui->statusBar->showMessage("Press OK");
    #if DISABLE_SLOT_MSG
    qDebug("OK");
    #endif
}

void MainWindow::onGroupButtonClicked(int)
{
    ui->statusBar->clearMessage();
    #if DISABLE_SLOT_MSG
    qDebug("Released!");
    #endif
}




