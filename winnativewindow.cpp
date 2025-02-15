#include "winnativewindow.h"

#include <tchar.h>
#include <windowsx.h>

#include <stdexcept>

#include "QtCore/QDebug"
#include "QtWidgets/QApplication"
#include "QtWidgets/QDesktopWidget"
#include "QtWidgets/QWidget"

namespace
{
	struct Context {
		HWND childWindow;
		QWidget* childWidget;
		int borderWidth = 0;
		QSize minimumSize;
		QSize maximumSize;

		Context() {
			childWindow = nullptr;
			childWidget = nullptr;
		}
	};

	LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Context *nativeWinContext = NULL;
		if (message == WM_CREATE)
		{
			CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			_ASSERT(pCreate);
			nativeWinContext = reinterpret_cast<Context *>(pCreate->lpCreateParams);
			_ASSERT(nativeWinContext);
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nativeWinContext));
		}
		else
		{
			nativeWinContext = reinterpret_cast<Context*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		if (!nativeWinContext) {
			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		switch (message)
		{
		case WM_NCCALCSIZE:
		{
			/*
				this kills the window frame and title bar we added with
				WS_THICKFRAME and WS_CAPTION;
			*/
			return 0;
		}
		/*
			If the parent window gets any close messages, send them over to
			XFramelessWidget and don't actually close here;
		*/
		case WM_CLOSE:
		{
			if (!nativeWinContext->childWindow)
			{
				break;
			}
			::ShowWindow(hwnd, FALSE);
			::SendMessage(nativeWinContext->childWindow, WM_CLOSE, 0, 0);
			// break will behave differently;
			return 0;
		}
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			break;
		}
		case WM_NCHITTEST:
		{
			if (!nativeWinContext->childWidget)
			{
				break;
			}
			QWidget* const childWindow = nativeWinContext->childWidget->window();
			if (!childWindow)
			{
				break;
			}
			const LONG borderWidth = nativeWinContext->borderWidth * childWindow->devicePixelRatio();
			RECT winrect;
			::GetWindowRect(hwnd, &winrect);
			long x = GET_X_LPARAM(lParam);
			long y = GET_Y_LPARAM(lParam);

			//  bottom left corner;
			if (x >= winrect.left && x < winrect.left + borderWidth &&
				y < winrect.bottom && y >= winrect.bottom - borderWidth)
			{
				return HTBOTTOMLEFT;
			}
			// bottom right corner;
			if (x < winrect.right && x >= winrect.right - borderWidth &&
				y < winrect.bottom && y >= winrect.bottom - borderWidth)
			{
				return HTBOTTOMRIGHT;
			}
			// top left corner;
			if (x >= winrect.left && x < winrect.left + borderWidth &&
				y >= winrect.top && y < winrect.top + borderWidth)
			{
				return HTTOPLEFT;
			}
			// top right corner;
			if (x < winrect.right && x >= winrect.right - borderWidth &&
				y >= winrect.top && y < winrect.top + borderWidth)
			{
				return HTTOPRIGHT;
			}
			// left border;
			if (x >= winrect.left && x < winrect.left + borderWidth)
			{
				return HTLEFT;
			}
			// right border;
			if (x < winrect.right && x >= winrect.right - borderWidth)
			{
				return HTRIGHT;
			}
			// bottom border;
			if (y < winrect.bottom && y >= winrect.bottom - borderWidth)
			{
				return HTBOTTOM;
			}
			// top border;
			if (y >= winrect.top && y < winrect.top + borderWidth)
			{
				return HTTOP;
			}

			/*
				If it wasn't a border but we still got the message,
				return HTCAPTION to allow click-dragging the window;
			*/
			return HTCAPTION;
		}
		/*
			When this native window changes size, it needs to manually resize
			the XFramelessWidget child;
		*/
		case WM_SIZE:
		{
			if (!nativeWinContext->childWidget)
			{
				break;
			}
			QWidget* childWindow = nativeWinContext->childWidget->window();
			RECT winRect;
			::GetClientRect(hwnd, &winRect);
			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(hwnd, &wp);
			int childTargetWidth = winRect.right / childWindow->devicePixelRatio();
			int childTargetHeight = winRect.bottom / childWindow->devicePixelRatio();
			nativeWinContext->childWidget->setGeometry(0, 0, childTargetWidth, childTargetHeight);
			break;
		}
		case WM_DPICHANGED:
		{
			RECT* const prcNewWindow = reinterpret_cast<RECT*>(lParam);
			::SetWindowPos(hwnd, NULL, prcNewWindow->left, prcNewWindow->top, 
				prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			break;
		}
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
			if (!nativeWinContext->maximumSize.isEmpty())
			{
				QPoint targetTopLeft = qApp->desktop()->availableGeometry().center() - 
					QPoint(nativeWinContext->maximumSize.width() / 2, 
					nativeWinContext->maximumSize.height() / 2);
				minMaxInfo->ptMaxPosition.x = targetTopLeft.x();
				minMaxInfo->ptMaxPosition.y = targetTopLeft.y();
				minMaxInfo->ptMaxTrackSize.x = nativeWinContext->maximumSize.width();
				minMaxInfo->ptMaxTrackSize.y = nativeWinContext->maximumSize.height();
			}

			if (!nativeWinContext->minimumSize.isEmpty())
			{
				minMaxInfo->ptMinTrackSize.x = nativeWinContext->minimumSize.width();
				minMaxInfo->ptMinTrackSize.y = nativeWinContext->minimumSize.height();
			}
			break;
		}
		case WM_SHOWWINDOW:
		{
			if (!nativeWinContext->childWidget)
			{
				break;
			}
			BOOL show = (BOOL)wParam;
			nativeWinContext->childWidget->setVisible(show);
			//::SendMessage((HWND)nativeWinContext->childWindow, WM_SHOWWINDOW, wParam, lParam);
			break;
		}
		}

		return ::DefWindowProc(hwnd, message, wParam, lParam);
	}
}

class WinNativeWindowPrivate final
{
public:
	WinNativeWindowPrivate(WinNativeWindow *q)
		: q_ptr(q)
	{}

	~WinNativeWindowPrivate()
	{
		::ShowWindow(hwnd_, SW_HIDE);
		::DestroyWindow(hwnd_);
	}

	void init()
	{
		HBRUSH windowBackground = ::CreateSolidBrush(RGB(255, 255, 255));
		HINSTANCE hInstance = ::GetModuleHandle(nullptr);
		WNDCLASSEX wcx = { 0 };
		wcx.cbSize = sizeof(WNDCLASSEX);
		wcx.style = CS_HREDRAW | CS_VREDRAW;
		wcx.hInstance = hInstance;
		wcx.lpfnWndProc = WndProc;
		wcx.cbClsExtra = 0;
		wcx.cbWndExtra = 0;
		wcx.lpszClassName = _T("WindowClass");
		wcx.hbrBackground = windowBackground;
		wcx.hCursor = ::LoadCursor(hInstance, IDC_ARROW);

		::RegisterClassEx(&wcx);
		if (FAILED(::RegisterClassEx(&wcx)))
		{
			throw std::runtime_error("Couldn't register window class");
		}

		// Create a native window with the appropriate style;
		const DWORD aero_borderless = WS_POPUP | WS_CAPTION | WS_SYSMENU | \
			WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CLIPCHILDREN;
		hwnd_ = ::CreateWindow(_T("WindowClass"), _T("WindowTitle"), aero_borderless, 0, 0,
			0, 0, 0, 0, hInstance, &this->context);
		if (!hwnd_)
		{
			throw std::runtime_error("couldn't create window because of reasons");
		}
		::SetWindowPos(hwnd_, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	}

	HWND getHWnd() const {
		return hwnd_;
	}

	void doSetChildWidget(QWidget* const childWgt)
	{
		if (!childWgt)
		{
			return;
		}

		this->context.childWidget = childWgt;
		this->context.childWindow = reinterpret_cast<HWND>(childWgt->winId());
	}

	void doSetGeometry(const int x, const int y, const int width, const int height)
	{
		::MoveWindow(hwnd_, x, y, width, height, TRUE);
	}

	void doSetBorderWidth(const int h)
	{
		this->context.borderWidth = h;
	}

	void doSetMinimumSize(const int width, const int height)
	{
		this->context.minimumSize.setWidth(width);
		this->context.minimumSize.setHeight(height);
	}

	int getMinimumWidth() const
	{
		return context.minimumSize.width();
	}

	int getMinimumHeight() const
	{
		return context.minimumSize.height();
	}

	void doSetMaximumSize(const int width, const int height)
	{
		this->context.maximumSize.setWidth(width);
		this->context.maximumSize.setHeight(height);
	}

	int getMaximumWidth() const
	{
		return context.maximumSize.width();
	}

	int getMaximumHeight() const
	{
		return context.maximumSize.height();
	}

private:
	WinNativeWindow *q_ptr;
	HWND hwnd_;

	Context context;
};

WinNativeWindow::WinNativeWindow()
	: d_ptr(new WinNativeWindowPrivate(this))
{
	d_ptr->init();
}

WinNativeWindow::~WinNativeWindow()
{
}

HWND WinNativeWindow::hwnd() const 
{
	return d_ptr->getHWnd();
}

void WinNativeWindow::setChildWidget(QWidget* const childWgt)
{
	d_ptr->doSetChildWidget(childWgt);
}

void WinNativeWindow::setGeometry(const int x, const int y, const int width, 
	const int height)
{
	d_ptr->doSetGeometry(x, y, width, height);
}

void WinNativeWindow::setBorderWidth(const int h)
{
	d_ptr->doSetBorderWidth(h);
}

void WinNativeWindow::setMinimumSize(const int width, const int height)
{
	d_ptr->doSetMinimumSize(width, height);
}

int WinNativeWindow::minimumWidth() const
{
	return d_ptr->getMinimumWidth();
}

int WinNativeWindow::minimumHeight() const
{
	return d_ptr->getMinimumHeight();
}

void WinNativeWindow::setMaximumSize(const int width, const int height)
{
	d_ptr->doSetMaximumSize(width, height);
}

int WinNativeWindow::maximumWidth() const
{
	return d_ptr->getMaximumWidth();
}

int WinNativeWindow::maximumHeight() const
{
	return d_ptr->getMaximumHeight();
}
