#include "xframelesswidget.h"

#include "QtCore/QDebug"
#include "QtGui/QFocusEvent"
#include "QtWidgets/QApplication"
#include "QtWidgets/QDesktopWidget"

#include "captionwidget.h"

#if defined(Q_OS_WIN)
#include <dwmapi.h>
#include <windowsx.h>
#include "winnativewindow.h"
#elif defined(Q_OS_MACOS)
#include "xutil_macos.h"
#elif defined(Q_OS_LINUX)
#include "xutil_linux.h"
#endif

namespace
{
#if defined(Q_OS_WIN)
#elif defined(Q_OS_LINUX)
	constexpr int ResizeHandleWidth = 10;
#endif
}

#if defined(Q_OS_WIN)

class XFramelessWidgetPrivate final 
{
public:
	explicit XFramelessWidgetPrivate(XFramelessWidget* q)
		:q_ptr(q),
		_nativeWindowHWnd(Q_NULLPTR),
		_nativeWindow(Q_NULLPTR),
		_prevFocus(Q_NULLPTR),
		_reenableParent(false),
		_capWgt(Q_NULLPTR) 
	{
		qDebug() << "XFramelessWidgetPrivate()";
	}

	~XFramelessWidgetPrivate() 
	{
		if (_nativeWindow)
		{
			delete _nativeWindow;
		}
		qDebug() << "~XFramelessWidgetPrivate()";
	}

	void setCaptionWidget(QWidget* const capWgt)
	{
		_capWgt = capWgt;
	}

	void init() {
		Q_Q(XFramelessWidget);
		_nativeWindow = new WinNativeWindow();
		_nativeWindowHWnd = _nativeWindow->hwnd();
		_borderWidth = _borderWidth * q->window()->devicePixelRatio();
		_nativeWindow->setBorderWidth(_borderWidth);
		q->setContentsMargins(0, 0, 0, 0);
		if (_nativeWindowHWnd)
		{
			q->setWindowFlags(Qt::FramelessWindowHint);
			q->setProperty("_q_embedded_native_parent_handle", (WId)_nativeWindowHWnd);
			const HWND childHWnd = reinterpret_cast<HWND>(q->winId());
			::SetWindowLong(childHWnd, GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
			::SetParent(childHWnd, _nativeWindowHWnd);
			QEvent e(QEvent::EmbeddingControl);
			QCoreApplication::sendEvent(q, &e);
		}
		_nativeWindow->setChildWidget(q);

		// Send the parent native window a WM_SIZE message to update the widget size;
		::SendMessage(_nativeWindowHWnd, WM_SIZE, 0, 0);

		// This code may be required for aero shadows on some versions of Windows;
		// to do;
		const MARGINS aero_shadow_on = { 6, 6, 6, 6 };
		::DwmExtendFrameIntoClientArea(_nativeWindowHWnd, &aero_shadow_on);
	}

	void setSystemTitle(const QString& title) 
	{
#if defined(UNICODE) || defined(_UNICODE)
		::SetWindowText(_nativeWindowHWnd, title.toStdWString().data());
#else
		::SetWindowText(_nativeWindowHWnd, title.toUtf8().data());
#endif
	}

	void setTitle(const QString& title) 
	{
		this->setSystemTitle(title);
	}

	void doChildEvent(QChildEvent *e)
	{
		Q_Q(XFramelessWidget);
		QObject *obj = e->child();
		if (obj->isWidgetType()) 
		{
			if (e->added()) 
			{
				if (obj->isWidgetType()) 
				{
					obj->installEventFilter(q);
				}
			}
			else if (e->removed() && _reenableParent) 
			{
				_reenableParent = false;
				::EnableWindow(_nativeWindowHWnd, true);
				obj->removeEventFilter(q);
			}
		}
	}

	bool getIsMaximized() const
	{
		WINDOWPLACEMENT wp = {};
		wp.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(_nativeWindowHWnd, &wp);
		return wp.showCmd == SW_MAXIMIZE;
	}

	void showParentWindow()
	{
		::ShowWindow(_nativeWindowHWnd, true);
		this->saveFocus();
	}

	/*! 
	 * to do:
	 * https://stackoverflow.com/questions/2382464/win32-full-screen-and-hiding-taskbar
	 */
	void showFullScreenParentWindow()
	{
	}

	void showMaximizedParentWindow()
	{
		::SendMessage(_nativeWindowHWnd, WM_SHOWWINDOW, TRUE, 0);
		::SendMessage(_nativeWindowHWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	}

	void showMinimizedParentWindow()
	{
		::SendMessage(_nativeWindowHWnd, WM_SHOWWINDOW, TRUE, 0);
		::SendMessage(_nativeWindowHWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}

	void showNormalParentWindow()
	{
		::SendMessage(_nativeWindowHWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	}

	void doResizeWork(int w, int h)
	{
		RECT rect;
		::GetWindowRect(_nativeWindowHWnd, &rect);
		::MoveWindow(_nativeWindowHWnd, rect.left, rect.top, w, h, TRUE);
	}

	void doShowCenter()
	{
		Q_Q(XFramelessWidget);
		const QWidget *child = q->findChild<QWidget*>();
		if (child && !child->isWindow()) 
		{
			qWarning("XFramelessWidget::center: Call this function only for "
				"QWinWidgets with toplevel children");
		}
		RECT r;
		::GetWindowRect(_nativeWindowHWnd, &r);
		const auto widgetWidth = r.right - r.left;
		const auto widgetHeight = r.bottom - r.top;
		const auto desktopRect = QApplication::desktop()->screenGeometry();
		const auto widgetX = (desktopRect.width() - widgetWidth) / 2;
		const auto widgetY = (desktopRect.height() - widgetHeight) / 2;
		this->doSetGeometry(widgetX, widgetY, widgetWidth, widgetHeight);
		q->show();
	}

	void doSetGeometry(int x, int y, int w, int h)
	{
		Q_Q(XFramelessWidget);
		_nativeWindow->setGeometry(
			  x * q->window()->devicePixelRatio()
			, y * q->window()->devicePixelRatio()
			, w * q->window()->devicePixelRatio()
			, h * q->window()->devicePixelRatio());
	}

	void doSetMinimumSize(const int w, const int h)
	{
		Q_Q(XFramelessWidget);
		_nativeWindow->setMinimumSize(w*q->window()->devicePixelRatio(),
			h*q->window()->devicePixelRatio());
	}

	void doSetMaximumSize(const int w, const int h)
	{
		Q_Q(XFramelessWidget);
		_nativeWindow->setMaximumSize(w*q->window()->devicePixelRatio(),
			h*q->window()->devicePixelRatio());
	}

	void hideParentWindow() {
		::ShowWindow(_nativeWindowHWnd, FALSE);
	}

	bool doNativeEvent(const QByteArray &ev, void *message, long *result)
	{
		Q_UNUSED(ev);
		Q_Q(XFramelessWidget);
		MSG* msg = reinterpret_cast<MSG*>(message);
		if (msg->message == WM_SETFOCUS)
		{
			Qt::FocusReason reason;
			if (::GetKeyState(VK_LBUTTON) < 0 || ::GetKeyState(VK_RBUTTON) < 0)
			{
				reason = Qt::MouseFocusReason;
			}
			else if (::GetKeyState(VK_SHIFT) < 0)
			{
				reason = Qt::BacktabFocusReason;
			}
			else
			{
				reason = Qt::TabFocusReason;
			}
			QFocusEvent e(QEvent::FocusIn, reason);
			QCoreApplication::sendEvent(q, &e);
		}
		/*!
			Pass NCHITTESTS on the window edges as determined by _borderWidth and
			_toolbarHeight through to the parent native window
		*/
		if (msg->message == WM_NCHITTEST)
		{
			RECT WindowRect;
			int x, y;

			::GetWindowRect(msg->hwnd, &WindowRect);
			x = GET_X_LPARAM(msg->lParam) - WindowRect.left;
			y = GET_Y_LPARAM(msg->lParam) - WindowRect.top;

			if (x >= _borderWidth && x <= WindowRect.right - WindowRect.left - _borderWidth && \
				y >= _borderWidth && y <= _toolbarHeight)
			{
				if (_capWgt && QApplication::widgetAt(QCursor::pos()) != _capWgt) 
				{
					return false;
				}

				/*!
					The mouse is over the toolbar area & is NOT over a child of the toolbar, 
					so pass this message through to the native window for HTCAPTION dragging
				*/
				*result = HTTRANSPARENT;
				return true;

			}
			else if (x < _borderWidth && y < _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (x > WindowRect.right - WindowRect.left - _borderWidth && y < _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (x > WindowRect.right - WindowRect.left - _borderWidth && y 
				> WindowRect.bottom - WindowRect.top - _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (x < _borderWidth && y > WindowRect.bottom - WindowRect.top - _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (x < _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (y < _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (x > WindowRect.right - WindowRect.left - _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}
			else if (y > WindowRect.bottom - WindowRect.top - _borderWidth)
			{
				*result = HTTRANSPARENT;
				return true;
			}

			return false;
		}
		return false;
	}
	
	void doUpdateToolBarHeight(const int h)
	{
		_toolbarHeight = h;
	}

	void doEventFilter(QObject *o, QEvent *e)
	{
		Q_Q(XFramelessWidget);
		QWidget* w = qobject_cast<QWidget*>(o);
		switch (e->type())
		{
		case QEvent::WindowDeactivate:
		{
			if (w->isModal() && w->isHidden())
			{
				::BringWindowToTop(_nativeWindowHWnd);
			}
			break;
		}
		case QEvent::Hide:
		{
			if (_reenableParent)
			{
				::EnableWindow(_nativeWindowHWnd, true);
				_reenableParent = false;
			}
			restoreFocus();

			if (w->testAttribute(Qt::WA_DeleteOnClose) && w->isWindow())
			{
				q->deleteLater();
			}
			break;
		}
		case QEvent::Show:
		{
			if (w->isWindow()) 
			{
				saveFocus();
				q->hide();
				if (w->isModal() && !_reenableParent) 
				{
					::EnableWindow(_nativeWindowHWnd, false);
					_reenableParent = true;
				}
			}
			break;
		}
		case QEvent::Close:
		{
			::SetActiveWindow(_nativeWindowHWnd);
			if (w->testAttribute(Qt::WA_DeleteOnClose))
			{
				q->deleteLater();
			}
			break;
		}
		default:
			break;
		}
	}

private:
	Q_DECLARE_PUBLIC(XFramelessWidget);
	XFramelessWidget *q_ptr;
	QWidget *_capWgt;

	WinNativeWindow* _nativeWindow;
	HWND _nativeWindowHWnd;

	HWND _prevFocus;
	bool _reenableParent;

	// Adjust this as you wish for # of pixels on the edges to show resize handles;
	int _borderWidth = 6;
	// Adjust this as you wish for # of pixels from the top to allow dragging the window;
	int _toolbarHeight = 40;

	void saveFocus()
	{
		if (!_prevFocus)
		{
			_prevFocus = ::GetFocus();
		}
		if (!_prevFocus)
		{
			_prevFocus = _nativeWindowHWnd;
		}
	}

	/*!
		Sets the focus to the window that had the focus before this widget
		was shown, or if there was no previous window, sets the focus to
		the parent window.
	*/
	void restoreFocus()
	{
		if (_prevFocus)
		{
			::SetFocus(_prevFocus);
		}
		else
		{
			::SetFocus(_nativeWindowHWnd);
		}
	}
};

#elif defined(Q_OS_MACOS)

class XFramelessWidgetPrivate final
{
public:
	explicit XFramelessWidgetPrivate(XFramelessWidget* q)
		:q_ptr(q)
	{
		qDebug() << "XFramelessWidgetPrivate()";
	}

	~XFramelessWidgetPrivate() {
		qDebug() << "~XFramelessWidgetPrivate()";
	}

	void init()
	{
		Q_Q(XFramelessWidget);
		xutils_macos::setupDialogTitleBar(q, true, true, true);
	}

	void doShowCenter()
	{
		Q_Q(XFramelessWidget);
		if (!qApp->desktop())
		{
			return;
		}
		q->move(qApp->desktop()->availableGeometry().center() - q->rect().center());
		q->show();
	}

private:
	Q_DECLARE_PUBLIC(XFramelessWidget);
	XFramelessWidget *q_ptr;
};

#elif defined(Q_OS_LINUX)

class XFramelessWidgetPrivate final
{
public:
	explicit XFramelessWidgetPrivate(XFramelessWidget* q)
		:q_ptr(q)
	{
		qDebug() << "XFramelessWidgetPrivate()";
	}

	~XFramelessWidgetPrivate() {
		qDebug() << "~XFramelessWidgetPrivate()";
	}

	void init()
	{
		Q_Q(XFramelessWidget);
		q->setWindowFlags(Qt::FramelessWindowHint);
		resizingCornerEdge = xutils_linux::CornerEdge::kInvalid;
		q->setMouseTracking(true);

		xutils_linux::SetMouseTransparent(q, true);
	}

	void doShowCenter()
	{
		Q_Q(XFramelessWidget);
		if (!qApp->desktop())
		{
			return;
		}
		q->move(qApp->desktop()->availableGeometry().center() - q->rect().center());
		q->show();
	}

	void doMouseMoveWork(QMouseEvent *event)
	{
		Q_Q(XFramelessWidget);
		const int x = event->x();
		const int y = event->y();
		if (resizingCornerEdge == xutils_linux::CornerEdge::kInvalid)
		{
			xutils_linux::UpdateCursorShape(q, x, y, q->layout()->contentsMargins(), ResizeHandleWidth);
		}
		xutils_linux::MoveWindow(q, event->button());
	}

	void doMousePressWork(QMouseEvent *event)
	{
		Q_Q(XFramelessWidget);
		const int x = event->x();
		const int y = event->y();
		if (event->button() == Qt::LeftButton)
		{
			const xutils_linux::CornerEdge ce = xutils_linux::GetCornerEdge(q, x, y, q->layout()->contentsMargins(),
				ResizeHandleWidth);
			if (ce != xutils_linux::CornerEdge::kInvalid)
			{
				resizingCornerEdge = ce;
				//send x11 move event dont send mouserrelease event
				xutils_linux::SendButtonRelease(q, event->pos(), event->globalPos());
				xutils_linux::StartResizing(q, QCursor::pos(), ce);
			}
		}
	}

	void doResizeWork(QResizeEvent *e)
	{
		Q_UNUSED(e);
		Q_Q(XFramelessWidget);
		xutils_linux::SetWindowExtents(q, q->layout()->contentsMargins(), ResizeHandleWidth);
	}

	void doMouseReleaseWork(QMouseEvent *event)
	{
		Q_UNUSED(event);
		resizingCornerEdge = xutils_linux::CornerEdge::kInvalid;
	}

private:
	Q_DECLARE_PUBLIC(XFramelessWidget);
	XFramelessWidget *q_ptr;
	xutils_linux::CornerEdge resizingCornerEdge;
	Qt::WindowFlags     dwindowFlags;
};
#endif

/*!
    \class XFramelessWidget
    \brief The XFramelessWidget class is a frameless widget implementation. 
	
	It work on Windows/macOS/Linux.Most importantly, it is based on TrueFramelessWindow 
	on https://github.com/dfct/TrueFramelessWindow. I refactor it much in my own way.
	On windows, XFramelessWidget keeps native window feature as many as possible, if not
	supported, we can change codes to support it. I'm not that familiar with macOS and Linux, 
	but it also works on bothplatforms. 
	
	You can use XFramelessWidget directly or use XFramelessWidgetWithCaption that  
	has a Caption/Title Widget. See CaptionIterface for more with XFramelessWidgetWithCaption.
	See sample.cpp for simple Demos;

	It may have bugs because it is tested rougly. If you find any bug, please try to 
	fix it or let me know. If you have any ideas about it, please let me know. Thanks in advance. 
*/
XFramelessWidget::XFramelessWidget(Qt::WindowFlags f /*= Qt::WindowFlags()*/)
    : QWidget(Q_NULLPTR, f),
	d_ptr(new XFramelessWidgetPrivate(this))
{
	qDebug() << "XFramelessWidget()";
	Q_D(XFramelessWidget);
	d->init();
}

/*!
    Destroys this object, freeing all allocated resources.
*/
XFramelessWidget::~XFramelessWidget()
{
	qDebug() << "~XFramelessWidget()";
}

void XFramelessWidget::setWindowTitle(const QString& title) {
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->setTitle(title);
#else
	QWidget::setWindowTitle(title);
#endif
}

void XFramelessWidget::hide()
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->hideParentWindow();
#else
	QWidget::hide();
#endif
}

/*!
    \sa showCentered()
*/
void XFramelessWidget::show()
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->showParentWindow();
#else
	QWidget::show();
#endif
}

bool XFramelessWidget::isMaximized() const
{
#if defined(Q_OS_WIN)
	Q_D(const XFramelessWidget);
	return d->getIsMaximized();
#else
	return QWidget::isMaximized();
#endif
}

void XFramelessWidget::resize(const QSize& sz)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doResizeWork(sz.width(), sz.height());
#else
	QWidget::resize(sz);
#endif
}

void XFramelessWidget::resize(int w, int h)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doResizeWork(w, h);
#else
	QWidget::resize(w, h);
#endif
}

void XFramelessWidget::showFullScreen()
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->showFullScreenParentWindow();
#else
	QWidget::showFullScreen();
#endif
}

void XFramelessWidget::showMaximized()
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->showMaximizedParentWindow();
#else
	QWidget::showMaximized();
#endif
}

void XFramelessWidget::showMinimized()
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->showMinimizedParentWindow();
#else
	QWidget::showMinimized();
#endif
}

void XFramelessWidget::showNormal()
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->showNormalParentWindow();
#else
	QWidget::showNormal();
#endif
}

void XFramelessWidget::showCenter()
{
	Q_D(XFramelessWidget);
	d->doShowCenter();
}

void XFramelessWidget::setGeometry(int x, int y, int w, int h)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doSetGeometry(x, y, w, h);
#else
	QWidget::setGeometry(x, y, w, h);
#endif
}

void XFramelessWidget::setMaximumSize(const QSize &sz)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doSetMaximumSize(sz.width(), sz.height());
#else
	QWidget::setMaximumSize(sz);
#endif
}

void XFramelessWidget::setMaximumSize(int w, int h)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doSetMaximumSize(w, h);
#else
	QWidget::setMaximumSize(w, h);
#endif
}

void XFramelessWidget::setMinimumSize(const QSize &sz)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doSetMinimumSize(sz.width(), sz.height());
#else
	QWidget::setMinimumSize(sz);
#endif
}

void XFramelessWidget::setMinimumSize(int w, int h)
{
#if defined(Q_OS_WIN)
	Q_D(XFramelessWidget);
	d->doSetMinimumSize(w, h);
#else
	QWidget::setMinimumSize(w, h);
#endif
}

#if defined(Q_OS_WIN)

/*!
 * Windows only. Set your caption widget to let Windows implementation handle
 * WM_NCHITTEST event as expected.
 * 
 * \param capWgt your caption object of QWidget*.
 */
void XFramelessWidget::setCaptionWidget(QWidget* const capWgt)
{
	Q_D(XFramelessWidget);
	d->setCaptionWidget(capWgt);
}

void XFramelessWidget::childEvent(QChildEvent *e)
{
	Q_D(XFramelessWidget);
	d->doChildEvent(e);

	QWidget::childEvent(e);
}

bool XFramelessWidget::eventFilter(QObject *o, QEvent *e)
{
	Q_D(XFramelessWidget);
	d->doEventFilter(o, e);

	return QWidget::eventFilter(o, e);
}

bool XFramelessWidget::nativeEvent(const QByteArray &ev, void *message, long *result)
{
	Q_D(XFramelessWidget);
	return d->doNativeEvent(ev, message, result);
}

void XFramelessWidget::updateToolBarHeight(const int h)
{
	Q_D(XFramelessWidget);
	d->doUpdateToolBarHeight(h);
}

void XFramelessWidget::focusInEvent(QFocusEvent *e)
{
    QWidget *candidate = this;

    switch (e->reason()) 
	{
    case Qt::TabFocusReason:
    case Qt::BacktabFocusReason:
	{
		while (!(candidate->focusPolicy() & Qt::TabFocus)) 
		{
			candidate = candidate->nextInFocusChain();
			if (candidate == this) 
			{
				candidate = 0;
				break;
			}
		}
		if (candidate) 
		{
			candidate->setFocus(e->reason());
			if (e->reason() == Qt::BacktabFocusReason || e->reason() == Qt::TabFocusReason) 
			{
				candidate->setAttribute(Qt::WA_KeyboardFocusChange);
				candidate->window()->setAttribute(Qt::WA_KeyboardFocusChange);
			}
			if (e->reason() == Qt::BacktabFocusReason)
			{
				QWidget::focusNextPrevChild(false);
			}
		}
		break;
	}
    default:
        break;
    }
}

bool XFramelessWidget::focusNextPrevChild(bool next)
{
	Q_D(XFramelessWidget);
    QWidget *curFocus = focusWidget();
    if (!next) 
	{
        if (!curFocus->isWindow()) 
		{
            QWidget *nextFocus = curFocus->nextInFocusChain();
            QWidget *prevFocus = 0;
            QWidget *topLevel = 0;
            while (nextFocus != curFocus) 
			{
                if (nextFocus->focusPolicy() & Qt::TabFocus) 
				{
                    prevFocus = nextFocus;
                    topLevel = 0;
                }
                nextFocus = nextFocus->nextInFocusChain();
            }

            if (!topLevel) 
			{
                return QWidget::focusNextPrevChild(false);
            }
        }
    } 
	else
	{
        QWidget *nextFocus = curFocus;
        while (1 && nextFocus != 0) 
		{
            nextFocus = nextFocus->nextInFocusChain();
            if (nextFocus->focusPolicy() & Qt::TabFocus) 
			{
                return QWidget::focusNextPrevChild(true);
            }
        }
    }

    ::SetFocus(d->_nativeWindowHWnd);

    return true;
}

#elif defined(Q_OS_LINUX)
void XFramelessWidget::mouseMoveEvent(QMouseEvent *event)
{
	Q_D(XFramelessWidget);
	d->doMouseMoveWork(event);

	QWidget::mouseMoveEvent(event);
}

void XFramelessWidget::mousePressEvent(QMouseEvent *event)
{
	Q_D(XFramelessWidget);
	d->doMousePressWork(event);

	QWidget::mousePressEvent(event);
}

void XFramelessWidget::resizeEvent(QResizeEvent *e)
{
	Q_D(XFramelessWidget);
	d->doResizeWork(e);

	QWidget::resizeEvent(e);
}

void XFramelessWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_D(XFramelessWidget);
	d->doMouseReleaseWork(event);

	QWidget::mouseReleaseEvent(event);
}
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
class XFramelessWidgetWithCaptionPrivate final
{
public:
	XFramelessWidgetWithCaptionPrivate(XFramelessWidgetWithCaption *q)
		: q_ptr(q)
		, _layout(Q_NULLPTR)
		, _capWgt(Q_NULLPTR)
	{
	}

	~XFramelessWidgetWithCaptionPrivate()
	{
	}

	void setCaptionTitle(const QString& title)
	{
		if (_capWgt)
		{
			_capWgt->setTitleText(title);
		}
	}

	CaptionIterface *captionItf() const {
		Q_ASSERT_X(_capWgt != Q_NULLPTR, __FUNCTION__, "caption widget is empty, "
			"try call setContentWidget or setContentLayout first.");
		return _capWgt;
	}

	void setContentWidget(QWidget* contentWidget) {
		Q_Q(XFramelessWidgetWithCaption);
		_layout = new QVBoxLayout(q);
		_layout->setContentsMargins(0, 0, 0, 0);
		_layout->setSpacing(0);
		_capWgt = new CaptionWidget(q);
		QObject::connect(_capWgt, SIGNAL(moreClicked(const QPoint&, const QPoint&)), q,
			SIGNAL(moreClicked(const QPoint&, const QPoint&)));
		QObject::connect(_capWgt, SIGNAL(minimizeClicked()), q, SLOT(onMinimized()));
		QObject::connect(_capWgt, SIGNAL(maximizeClicked()), q, SLOT(onMaximizeToggle()));
		QObject::connect(_capWgt, SIGNAL(closed()), q, SLOT(onClosed()));

		_layout->addWidget(_capWgt);
		_layout->addWidget(contentWidget);
		contentWidget->setVisible(true);
		_capWgt->raise();

#if defined(Q_OS_WIN)
		q->setCaptionWidget(_capWgt);
		q->updateToolBarHeight(_capWgt->height() * q->window()->devicePixelRatio());
#elif defined(Q_OS_LINUX)
#endif
	}

	void setContentLayout(QLayout* layout) {
		Q_ASSERT_X(layout!= Q_NULLPTR, __FUNCTION__, "layout cannt be empty");
		Q_Q(XFramelessWidgetWithCaption);
		_layout = new QVBoxLayout(q);
		_layout->setContentsMargins(0, 0, 0, 0);
		_layout->setSpacing(0);
		_capWgt = new CaptionWidget(q);
		QObject::connect(_capWgt, SIGNAL(moreClicked(const QPoint&, const QPoint&)), q, SIGNAL(moreClicked(const QPoint&, const QPoint&)));
		QObject::connect(_capWgt, SIGNAL(minimized()), q, SLOT(onMinimized()));
		QObject::connect(_capWgt, SIGNAL(maximizeClicked()), q, SLOT(onMaximizeClicked()));
		QObject::connect(_capWgt, SIGNAL(closed()), q, SLOT(onClosed()));

		_layout->addWidget(_capWgt);
		_layout->addLayout(layout);
		_capWgt->raise();

#if defined(Q_OS_WIN)
		q->setCaptionWidget(_capWgt);
		q->updateToolBarHeight(_capWgt->height() * q->window()->devicePixelRatio());
#elif defined(Q_OS_LINUX)
#endif
	}

	void doSetMainLayoutMargins(const int left, const int top, const int right,
		const int bottom)
	{
		_layout->setContentsMargins(left, top, right, bottom);
	}

	void doSetMainLayoutSpacing(const int spacing)
	{
		_layout->setSpacing(spacing);
	}

	void doMaximizeToggle()
	{
		Q_Q(XFramelessWidgetWithCaption);
		if (q->isMaximized())
		{
			q->showNormal();
		}
		else
		{
			q->showMaximized();
		}
	}

	void doMinimize()
	{
		Q_Q(XFramelessWidgetWithCaption);
		q->showMinimized();
	}

	void doClose()
	{
		Q_Q(XFramelessWidgetWithCaption);
		emit q->closeRequested(XFramelessWidgetWithCaption::QPrivateSignal());
	}

#if defined(Q_OS_WIN)
	bool doNativeEvent(const QByteArray &ev, void *message, long *result)
	{
		Q_UNUSED(ev);
		Q_UNUSED(result);
		Q_Q(XFramelessWidgetWithCaption);
		MSG *msg = (MSG *)message;
		if (msg->message == WM_CLOSE)
		{
			emit q->closeRequested(XFramelessWidgetWithCaption::QPrivateSignal());
		}

		return false;
	}
#endif

private:
	Q_DECLARE_PUBLIC(XFramelessWidgetWithCaption);
	XFramelessWidgetWithCaption *q_ptr;
	QVBoxLayout *_layout;
	CaptionWidget *_capWgt;
};

XFramelessWidgetWithCaption::XFramelessWidgetWithCaption()
	: XFramelessWidget()
	, d_ptr(new XFramelessWidgetWithCaptionPrivate(this))
{
}

XFramelessWidgetWithCaption::~XFramelessWidgetWithCaption()
{
}

void XFramelessWidgetWithCaption::setWindowTitle(const QString& title)
{
	Q_D(XFramelessWidgetWithCaption);
	d->setCaptionTitle(title);
	XFramelessWidget::setWindowTitle(title);
}

CaptionIterface *XFramelessWidgetWithCaption::captionItf()
{
	Q_D(XFramelessWidgetWithCaption);
	return d->captionItf();
}

void XFramelessWidgetWithCaption::setContentWidget(QWidget* contentWidget)
{
	Q_D(XFramelessWidgetWithCaption);
	d->setContentWidget(contentWidget);
}

void XFramelessWidgetWithCaption::setContentLayout(QLayout* layout)
{
	Q_D(XFramelessWidgetWithCaption);
	d->setContentLayout(layout);
}

void XFramelessWidgetWithCaption::setMainLayoutMargins(const int left, const int top, const int right,
	const int bottom)
{
	Q_D(XFramelessWidgetWithCaption);
	d->doSetMainLayoutMargins(left, top, right, bottom);
}

void XFramelessWidgetWithCaption::setMainLayoutSpacing(const int spacing)
{
	Q_D(XFramelessWidgetWithCaption);
	d->doSetMainLayoutSpacing(spacing);
}

void XFramelessWidgetWithCaption::onMaximizeToggle()
{
	Q_D(XFramelessWidgetWithCaption);
	d->doMaximizeToggle();
}

void XFramelessWidgetWithCaption::onMinimized()
{
	Q_D(XFramelessWidgetWithCaption);
	d->doMinimize();
}

void XFramelessWidgetWithCaption::onClosed()
{
	Q_D(XFramelessWidgetWithCaption);
	d->doClose();
}

#if defined(Q_OS_WIN)
bool XFramelessWidgetWithCaption::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	Q_D(XFramelessWidgetWithCaption);
	if (d->doNativeEvent(eventType, message, result)) 
	{
		return true;
	}
	return XFramelessWidget::nativeEvent(eventType, message, result);
}
#endif

#endif
