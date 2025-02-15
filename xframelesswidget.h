#ifndef XFRAMELESSWIDGET_H
#define XFRAMELESSWIDGET_H

#ifdef X_FRAMELESS_WIDGET_SHARED
#define X_FRAMELESS_WIDGET_EXPORT Q_DECL_EXPORT
#else
#define X_FRAMELESS_WIDGET_EXPORT Q_DECL_IMPORT
#endif

#include "QtWidgets/QWidget"
#include "QtWidgets/QVBoxLayout"

#include "captionitf.h"

class XFramelessWidgetPrivate;

class X_FRAMELESS_WIDGET_EXPORT XFramelessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit XFramelessWidget(Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~XFramelessWidget();

	// reimplement QWidget functions;
	void hide();
	bool isMaximized() const;
	void resize(const QSize& sz);
	void resize(int w, int h);
	void setGeometry(int x, int y, int w, int h);
	void setMaximumSize(const QSize &sz);
	void setMaximumSize(int w, int h);
	void setMinimumSize(const QSize &sz);
	void setMinimumSize(int w, int h);
	void setWindowTitle(const QString& title);
	void show();
	void showFullScreen();
	void showMaximized();
	void showMinimized();
	void showNormal();

	// new feature functions;
	void showCenter();

#if defined(Q_OS_WIN)
	void setCaptionWidget(QWidget* const capWgt);
#elif defined(Q_OS_MACOS)
#elif defined(Q_OS_LINUX)
#endif

protected:
#if defined(Q_OS_WIN)
    void childEvent( QChildEvent *e ) override;
    bool eventFilter( QObject *o, QEvent *e ) override;
	bool focusNextPrevChild(bool next) override;
	void focusInEvent(QFocusEvent *e) override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
	void updateToolBarHeight(const int h);
#elif defined(Q_OS_LINUX)
	void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;
#endif

private:
	Q_DECLARE_PRIVATE(XFramelessWidget);
	QScopedPointer<XFramelessWidgetPrivate> d_ptr;
};

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
class XFramelessWidgetWithCaptionPrivate;
class X_FRAMELESS_WIDGET_EXPORT XFramelessWidgetWithCaption: public XFramelessWidget
{
	Q_OBJECT

public:
	explicit XFramelessWidgetWithCaption();
	virtual ~XFramelessWidgetWithCaption();

	void setWindowTitle(const QString& title);

	CaptionIterface *captionItf();
	void setContentWidget(QWidget* contentWidget);
	void setContentLayout(QLayout* layout);
	void setMainLayoutMargins(const int left, const int top, const int right, const int bottom);
	void setMainLayoutSpacing(const int spacing);

	Q_SIGNAL void closeRequested(QPrivateSignal);
	Q_SIGNAL void moreClicked(const QPoint& wgtPos, const QPoint& globalPos);

	Q_SLOT void onMaximizeToggle();
	Q_SLOT void onMinimized();
	Q_SLOT void onClosed();

protected:
#if defined(Q_OS_WIN)
	bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif // defined(Q_OS_WIN)

private:
	Q_DECLARE_PRIVATE(XFramelessWidgetWithCaption);
	QScopedPointer<XFramelessWidgetWithCaptionPrivate> d_ptr;
};
#endif

#endif // XFRAMELESSWIDGET_H
