//
// Created by kj16609 on 4/17/23.
//

#include "ImageDelegate.hpp"

#include <filesystem>

#include <QPainter>

#include "atlas/config.hpp"

void ImageDelegate::paint( QPainter* painter, const QStyleOptionViewItem& item, const QModelIndex& index ) const
{
	ZoneScoped;
	const std::filesystem::path path { index.data( Qt::DisplayRole ).value< std::filesystem::path >() };

	if ( !std::filesystem::exists( path ) ) return;

	QPixmap pixmap { path.c_str() };

	pixmap = pixmap.scaled( item.rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

	painter->drawPixmap( item.rect.center() - pixmap.rect().center(), pixmap );
	painter->drawRect( item.rect );
}

QSize ImageDelegate::
	sizeHint( [[maybe_unused]] const QStyleOptionViewItem& item, [[maybe_unused]] const QModelIndex& index ) const
{
	ZoneScoped;
	return { config::grid_ui::gridSizeX::get(), config::grid_ui::gridSizeY::get() };
}
