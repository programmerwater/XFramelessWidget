#include "sample.h"

#include "QtCore/QDateTime"
#include "QtCore/QDebug"
#include "QtCore/QFile"
#include "QtCore/QMutex"
#include "QtWidgets/QApplication"
#include "QtWidgets/QLabel"
#include "QtWidgets/QPushButton"
#include "QtWidgets/QTextEdit"

#include "scopeguard.h"
#include "xframelesswidget.h"

QFile* getLogFile() {
	static QFile* file = new QFile(QString("xframelesswidget_sample.log"));
	if (!file) return Q_NULLPTR;
	return file;
}

void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	static QMutex messageMutex;

	const auto logFile = getLogFile();
	if (!logFile) return;

	QString msgType = QString("[Unknown]");
	switch (type) {
    case QtInfoMsg:
	case QtDebugMsg:
        msgType = QString("[Info]");
		break;
	case QtWarningMsg:
		msgType = QString("[Warning]");
		break;
	case QtCriticalMsg:
		msgType = QString("[Critical]");
		break;
	case QtFatalMsg:
		msgType = QString("[Fatal]");
	}

	const QDateTime dateTime = QDateTime::currentDateTime();
	const auto dateTimeString = dateTime.toString("[yyyy-MM-dd_hh:mm:ss.zzz]");
	const auto fileFunctionLineString = QString("codeLocation:%1,%2,%3")
		.arg(context.file).arg(context.line).arg(context.function);
	auto finalMsg = dateTimeString + msgType + " " + msg + " " + fileFunctionLineString;
	{
		QMutexLocker locker(&messageMutex);
		logFile->write(finalMsg.toUtf8() + "\n");
		logFile->flush();
	}
}

bool enableLog(const bool enable) {
	const auto logFile = getLogFile();
	if (!logFile) return false;
	if (enable) {
		if (!logFile->open(QFile::WriteOnly)) return false;
		qInstallMessageHandler(myMessageHandler);
	}
	else {
		qInstallMessageHandler(Q_NULLPTR);
		if (logFile->isOpen()) logFile->close();
	}

	return true;
}

XFramelessWidgetChild::XFramelessWidgetChild()
	: XFramelessWidget()
{
	QVBoxLayout *tempLayout = new QVBoxLayout;
	tempLayout->setSpacing(0);
	QPushButton *tempBtn = new QPushButton(this);
	tempBtn->setText("btn");
	tempBtn->raise();
	tempLayout->addWidget(tempBtn);
	QPushButton *tempBtn2 = new QPushButton(this);
	tempBtn2->setText("btn2");
	tempLayout->addWidget(tempBtn2);
	QLabel* tempLbl = new QLabel(this);
	tempLbl->setText("text");
	tempLbl->setFixedHeight(30);
	tempLayout->addWidget(tempLbl);
	tempLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	this->setLayout(tempLayout);

#if defined(Q_OS_WIN)
	this->setCaptionWidget(this);
#elif defined(Q_OS_MACOS)
#elif defined(Q_OS_LINUX)
#endif
}

XFramelessWidgetChild::~XFramelessWidgetChild()
{

}

int main(int argc, char *argv[])
{
	ScopeGuard s([] {
		qDebug() << "main exit";
#if defined(_MSC_VER) && defined(NDEBUG)
		enableLog(false);
#endif // _NDEBUG
	});
	s.used();
	enableLog(true);

    /*
		This has the app draw at HiDPI scaling on HiDPI displays, usually two
		pixels for every one logical pixel
	*/
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    /*
		This has QPixmap images use the @2x images when available
		See this bug for more details on how to get this right: 
		https://bugreports.qt.io/browse/QTBUG-44486#comment-327410
	*/
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    /* 
		On Windows, the widget needs to be encapsulated in a native window for
		frameless rendering
		In this case, XFramelessWidget #includes "Widget.h", creates it, and
		adds it to a layout
	*/
	XFramelessWidgetWithCaption w;
	QObject::connect(&w, SIGNAL(closeRequested()), &app, SLOT(quit()));
	QTextEdit *txtEdit = new QTextEdit;
	txtEdit->setStyleSheet("background: transparent;");
    w.setContentWidget(txtEdit);
	w.setWindowTitle("longlonglonglonglonglonglong");
    w.resize(1024, 768);
	w.showCenter();
#endif // defined(Q_OS_WIN) || defined(Q_OS_MACOS)

	XFramelessWidgetChild child;
	child.resize(400, 300);
	child.showCenter();

    return app.exec();
}
