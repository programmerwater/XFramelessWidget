#include "captionwidget.h"
#include "ui_captionwidget.h"

#include "QtGui/QFont"
#include "QtGui/QPainter"
#include "QtWidgets/QLabel"
#include "QtWidgets/QStyleOption"
#include "QtWidgets/QPushButton"

CaptionWidget::CaptionWidget(QWidget * parent) 
	: QWidget(parent), ui(new Ui::CaptionWidget)
{
	ui->setupUi(this);

	ui->titleLbl->setText("");
	ui->titleLbl->setObjectName("titleLbl");
	ui->titleLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	QFont ft = ui->titleLbl->font();
	ft.setPixelSize(12);
	ft.setBold(true);
	ui->titleLbl->setFont(ft);

    ft = ui->btnMore->font();
	ft.setBold(true);
    ui->btnMore->setFont(ft);
	ui->btnMinimize->setText("");
	ui->btnMaximize->setText("");
	ui->btnMaximize->setFocusPolicy(Qt::NoFocus);
	ui->btnClose->setText("");

    connect(ui->btnMore, SIGNAL(clicked()), this, SLOT(moreButtonClicked()));
	connect(ui->btnMinimize, SIGNAL(clicked()), this, SIGNAL(minimizeClicked()));
	connect(ui->btnMaximize, SIGNAL(clicked()), this, SIGNAL(maximizeClicked()));
	connect(ui->btnClose, SIGNAL(clicked()), this, SIGNAL(closed()));
}

CaptionWidget::~CaptionWidget() {
	delete ui;
}

void CaptionWidget::paintEvent(QPaintEvent *event)
{
	/* For setStyleSheet function well, why does it need; */
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

	QWidget::paintEvent(event);
}

QWidget* CaptionWidget::widget()
{
	return this;
}

void CaptionWidget::showIcon(bool b) {
	ui->iconLbl->setVisible(b);
}

void CaptionWidget::setIcon(const QPixmap& pixmap) {
	ui->iconLbl->setPixmap(pixmap);
}

void CaptionWidget::showTitleText(bool b) {
	ui->titleLbl->setVisible(b);
}

void CaptionWidget::setTitleText(const QString& text) {
	ui->titleLbl->setText(text);
}

void CaptionWidget::insertWidget(int index, QWidget *widget, int stretch /*= 0*/,
	Qt::Alignment alignment /*= Qt::Alignment()*/) {
	QHBoxLayout * hboxlayout = qobject_cast<QHBoxLayout *>(this->layout());
	Q_ASSERT(hboxlayout);
	hboxlayout->insertWidget(index, widget, stretch, alignment);
}

int CaptionWidget::indexOfLogo() const {
	Q_ASSERT(this->layout());
	return this->layout()->indexOf(ui->iconLbl);
}

int CaptionWidget::indexOfTitleText() const {
	Q_ASSERT(this->layout());
	return this->layout()->indexOf(ui->titleLbl);
}

int CaptionWidget::indexOfMoreButton() const {
	Q_ASSERT(this->layout());
	return this->layout()->indexOf(ui->btnMore);
}

int CaptionWidget::indexOfWidget(QWidget * const wgt) const {
	Q_ASSERT(this->layout());
	return this->layout()->indexOf(wgt);
}

void CaptionWidget::showMoreButton(bool b) {
    ui->btnMore->setVisible(b);
}

void CaptionWidget::showMinimizeButton(bool b) {
	ui->btnMinimize->setVisible(b);
}

void CaptionWidget::showMaximizeButton(bool b) {
	ui->btnMaximize->setVisible(b);
}

void CaptionWidget::showCloseButton(bool b) {
	ui->btnClose->setVisible(b);
}

void CaptionWidget::changeLeftSpacerSize(const int w, const int h) {
	ui->leftSpacer->changeSize(w, h);
}

void CaptionWidget::changeRightSpacerSize(const int w, const int h) {
	ui->rightSpacer->changeSize(w, h);
}

void CaptionWidget::moreButtonClicked() {
    QPoint pos_ = ui->btnMore->pos() + QPoint(0, ui->btnMore->height());
    emit moreClicked(pos_, this->mapToGlobal(pos_));
}