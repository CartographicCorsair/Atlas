//
// Created by kj16609 on 7/21/23.
//

#include "Notification.hpp"

#include <moc_Notification.cpp>

#include <QMouseEvent>

void Notification::mousePressEvent( QMouseEvent* event )
{
	if ( event->button() == Qt::RightButton )
	{
		event->accept();
		this->close();
		return;
	}
	else
		QDialog::mousePressEvent( event );
}

Notification::Notification( QWidget* parent ) : QDialog( parent, Qt::Tool | Qt::FramelessWindowHint )
{
	connect( this, &Notification::selfClose, this, &Notification::selfCloseTrigger );
}

void Notification::selfCloseTrigger()
{
	emit selfClosePtr( this );
}
