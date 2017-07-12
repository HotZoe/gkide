#ifndef NEOVIM_QT_MAINWINDOW
#define NEOVIM_QT_MAINWINDOW

#include <QMainWindow>
#include <QStackedWidget>
#include "snail/libs/nvimcore/nvimconnector.h"
#include "snail/libs/nvimcore/errorwidget.h"
#include "snail/libs/nvimcore/shell.h"

namespace SnailNvimQt {

class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	enum DelayedShow {
		Disabled,
		Normal,
		Maximized,
		FullScreen,
	};

	MainWindow(NvimConnector *, QWidget *parent=0);
	bool neovimAttached() const;
	Shell* shell();
public slots:
	void delayedShow(DelayedShow type=DelayedShow::Normal);
signals:
	void neovimAttached(bool);
protected:
	virtual void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
	virtual void changeEvent(QEvent *ev) Q_DECL_OVERRIDE;
private slots:
	void neovimSetTitle(const QString &title);
	void neovimWidgetResized();
	void neovimMaximized(bool);
	void neovimFullScreen(bool);
	void neovimGuiCloseRequest();
	void neovimExited(int status);
	void neovimError(NvimConnector::NeovimError);
	void reconnectNeovim();
	void showIfDelayed();
	void neovimAttachmentChanged(bool);
private:
	void init(NvimConnector *);
        NvimConnector *m_nvim;
	ErrorWidget *m_errorWidget;
	Shell *m_shell;
	DelayedShow m_delayedShow;
	QStackedWidget m_stack;
};

} // [Namespace] SnailNvimQt

#endif
