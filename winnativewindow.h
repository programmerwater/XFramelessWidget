#ifndef __WINNATIVEWINDOW_H__
#define __WINNATIVEWINDOW_H__

#include <windows.h>

#include <memory>

class QWidget;
class WinNativeWindowPrivate;
class WinNativeWindow
{
public:
    WinNativeWindow();
	~WinNativeWindow();

	void setChildWidget(QWidget* const childWgt);

	HWND hwnd() const;

	void setBorderWidth(const int h);

    /*! \internal
		These six functions exist to restrict native window resizing to whatever 
		you want your app minimum/maximum size to be;
	*/
    void setMinimumSize(const int width, const int height);
    int minimumHeight() const;
    int minimumWidth() const;

    void setMaximumSize(const int width, const int height);
    int maximumHeight() const;
    int maximumWidth() const;
    void setGeometry(const int x, const int y, const int width, const int height);

private:
	std::unique_ptr<WinNativeWindowPrivate> d_ptr;
};

#endif // __WINNATIVEWINDOW_H__
