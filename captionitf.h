#ifndef CAPTIONITF_H
#define CAPTIONITF_H

#include "QtCore/QString"
#include "QtGui/QPixmap"

class QWidget;

class CaptionIterface
{
public:
	virtual QWidget* widget() = 0;
	virtual void showIcon(bool b) = 0;
	virtual void setIcon(const QPixmap& pixmap) = 0;
	virtual void showTitleText(bool b) = 0;
	virtual void setTitleText(const QString& text) = 0;
	virtual void insertWidget(int index, QWidget *widget, int stretch = 0,  
		Qt::Alignment alignment = Qt::Alignment()) = 0;
	virtual int indexOfLogo() const = 0;
	virtual int indexOfTitleText() const = 0;
	virtual int indexOfMoreButton() const = 0;
	virtual int indexOfWidget(QWidget * const wgt) const = 0;
	virtual void showMoreButton(bool b) = 0;
	virtual void showMinimizeButton(bool b) = 0;
	virtual void showMaximizeButton(bool b) = 0;
	virtual void showCloseButton(bool b) = 0;
	virtual void changeLeftSpacerSize(const int w, const int h) = 0;
	virtual void changeRightSpacerSize(const int w, const int h) = 0;
};

#endif // CAPTIONITF_H;