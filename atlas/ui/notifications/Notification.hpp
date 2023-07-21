//
// Created by kj16609 on 7/21/23.
//

#ifndef ATLASGAMEMANAGER_NOTIFICATION_HPP
#define ATLASGAMEMANAGER_NOTIFICATION_HPP

#include <QDialog>
#include <QWidget>

class Notification : public QDialog
{
	Q_OBJECT
	Q_DISABLE_COPY_MOVE( Notification )

  public:

	Notification( QWidget* parent = nullptr );

	void mousePressEvent( QMouseEvent* event ) override;

  public:

  private slots:
	void selfCloseTrigger();

  signals:
	void selfClose();
	void selfClosePtr( Notification* ptr );
};

#endif //ATLASGAMEMANAGER_NOTIFICATION_HPP
