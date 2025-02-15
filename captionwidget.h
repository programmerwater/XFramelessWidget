#ifndef CAPTIONWIDGET_H
#define CAPTIONWIDGET_H

#ifdef X_FRAMELESS_WIDGET_SHARED
#define X_FRAMELESS_WIDGET_EXPORT Q_DECL_EXPORT
#else
#define X_FRAMELESS_WIDGET_EXPORT Q_DECL_IMPORT
#endif

#include "captionitf.h"

#include <QtCore/QPoint>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QLabel;
class QPushButton;

namespace Ui {
	class CaptionWidget;
}

class X_FRAMELESS_WIDGET_EXPORT CaptionWidget : public QWidget, public CaptionIterface {
	Q_OBJECT

public:
	CaptionWidget(QWidget * parent = Q_NULLPTR);
	~CaptionWidget();

	QWidget* widget() override;
	void showIcon(bool b) override;
	void setIcon(const QPixmap& pixmap) override;
	void showTitleText(bool b) override;
	void setTitleText(const QString& text);
	void insertWidget(int index, QWidget *widget, int stretch = 0,
		Qt::Alignment alignment = Qt::Alignment()) override;
	int indexOfLogo() const override;
	int indexOfTitleText() const override;
	int indexOfMoreButton() const override;
	int indexOfWidget(QWidget * const wgt) const override;
	void showMoreButton(bool b) override;
	void showMinimizeButton(bool b) override;
	void showMaximizeButton(bool b) override;
	void showCloseButton(bool b) override;
	void changeLeftSpacerSize(const int w, const int h) override;
	void changeRightSpacerSize(const int w, const int h) override;

	Q_SIGNAL void moreClicked(const QPoint& wgtPos, const QPoint& globalPos);
	Q_SIGNAL void minimizeClicked();
	Q_SIGNAL void maximizeClicked();
	Q_SIGNAL void closed();

protected:
	virtual void paintEvent(QPaintEvent *event);

private:
	Ui::CaptionWidget *ui;

	Q_SLOT void moreButtonClicked();
};
#endif