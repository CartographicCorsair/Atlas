//
// Created by kj16609 on 3/29/23.
//

#include "RecordBannerDelegate.hpp"

#include <QMenu>
#include <QPainter>
#include <QPixmapCache>

#include <tracy/Tracy.hpp>

#include "atlas/config.hpp"
#include "atlas/database/GameMetadata.hpp"
#include "atlas/database/Record.hpp"
#include "atlas/utils/QImageBlur.hpp"

void RecordBannerDelegate::paint( QPainter* painter, const QStyleOptionViewItem& options, const QModelIndex& index )
	const
{
	ZoneScoped;
	painter->save();

	//Draw banner if present
	const Record record { index.data().value< Record >() };

	const auto banner_size { m_grid_size };

	const SCALE_TYPE aspect_ratio { m_scale_type };
	const int stripe_height { m_strip_height };
	const int overlay_opacity { m_overlay_opacity };
	const bool enable_top_overlay { m_enable_top_overlay };
	const bool enable_bottom_overlay { m_enable_bottom_overlay };
	//const int w_width { painter->window().width() };
	//const int item_count { w_width / banner_size.width() };

	//const int x_offset { ( w_width - ( ( item_count + 1 ) * m_grid_spacing ) ) / item_count };
	//printf( "Item Count:%d offset:%d\n", item_count, x_center );

	QRect options_rect { options.rect.x(), options.rect.y(), banner_size.width(), banner_size.height() };
	//options_rect = options_rect.adjusted( 5, 5, 5, 5 );

	QPixmap pixmap = record->getBanner( banner_size.width(), banner_size.height(), aspect_ratio );

	//TODO: Modify Options rect so items are centered

	//Check if we need to add blur background. Draw behind original image
	if ( aspect_ratio == FIT_BLUR_EXPANDING )
	{
		pixmap = blurToSize(
			pixmap, banner_size.width(), banner_size.height(), m_feather_radius, m_blur_radius, m_blur_type );
	}

	//If the image needs to be centered then calculate it. because Qrect only does not take doubles, the center will not be exact.
	const int x_m { aspect_ratio == KEEP_ASPECT_RATIO ? ( banner_size.width() - pixmap.width() ) / 2 : 0 };
	const int y_m { aspect_ratio == KEEP_ASPECT_RATIO ? ( banner_size.height() - pixmap.height() ) / 2 : 0 };
	const QRect pixmap_rect { options_rect.x() + x_m, options_rect.y() + y_m, pixmap.width(), pixmap.height() };

	//Draw Image
	painter->drawPixmap( pixmap_rect, pixmap );

	//Click & Selectec event
	//TODO: add ability to change selected color.
	if ( options.state & QStyle::State_MouseOver ) painter->fillRect( options_rect, QColor( 0, 0, 255, 50 ) );

	//Reset the current brush
	painter->setBrush( Qt::NoBrush );
	QPen pen;
	//Used for border around the grid capsule
	pen.setBrush( m_enable_capsule_border ? Qt::black : Qt::transparent );
	pen.setWidth( 1 );
	pen.setStyle( Qt::SolidLine );
	painter->setPen( pen );
	painter->drawRect( options_rect );

	//Draw top
	const QRect top_rect { options_rect.topLeft().x(), options_rect.topLeft().y(), banner_size.width(), stripe_height };

	painter->fillRect( top_rect, QColor( 0, 0, 0, enable_top_overlay ? overlay_opacity : 0 ) );

	//Draw bottom
	const QRect bottom_rect { options_rect.bottomLeft().x(),
		                      options_rect.bottomLeft().y() - stripe_height + 1,
		                      banner_size.width(),
		                      stripe_height };

	painter->fillRect( bottom_rect, QColor( 0, 0, 0, enable_bottom_overlay ? overlay_opacity : 0 ) );

	//set Pen and Font for default text
	QFont font;
	font.setPixelSize( m_font_size );
	font.setFamily( m_font_family );
	painter->setFont( font );
	painter->setPen( qRgb( 210, 210, 210 ) );
	//TODO: Add so the user will be able to change the color. This is the default for all pallets

	//Draw Title
	this->drawText( painter, options_rect, stripe_height, m_title_location, record->getTitle() );
	//Draw Engine
	this->drawText( painter, options_rect, stripe_height, m_engine_location, record->getEngine() );
	//Draw Version
	const auto& latest { record->getLatestVersion() };
	if ( latest.has_value() )
		this->drawText( painter, options_rect, stripe_height, m_version_location, latest->getVersionName() );
	else
		this->drawText( painter, options_rect, stripe_height, m_version_location, "No Version" );
	//Draw Creator
	this->drawText( painter, options_rect, stripe_height, m_creator_location, record->getCreator() );

	painter->restore();
}

QSize RecordBannerDelegate::
	sizeHint( [[maybe_unused]] const QStyleOptionViewItem& item, [[maybe_unused]] const QModelIndex& index ) const
{
	ZoneScoped;
	return m_grid_size;
}

void RecordBannerDelegate::
	drawText( QPainter* painter, const QRect& rect, const int strip_size, const LOCATION location, const QString& str )
		const
{
	ZoneScoped;
	if ( location != NONE )
	{
		const QSize size { rect.width(), strip_size };

		switch ( location )
		{
			case TOP_LEFT:
				{
					const QRect text_rect { rect.topLeft() + QPoint( 10, 0 ), size };
					painter->drawText( text_rect, Qt::AlignLeft | Qt::AlignVCenter, str );
					return;
				}
			case TOP_CENTER:
				{
					const QRect text_rect { rect.topLeft(), size };
					painter->drawText( text_rect, Qt::AlignCenter, str );
					return;
				}
			case TOP_RIGHT:
				{
					const QRect text_rect { rect.topLeft() + QPoint( -10, 0 ), size };
					painter->drawText( text_rect, Qt::AlignRight | Qt::AlignVCenter, str );
					return;
				}
			case BOTTOM_LEFT:
				{
					const QRect text_rect { rect.bottomLeft() + QPoint( 10, -strip_size ), size };
					painter->drawText( text_rect, Qt::AlignLeft | Qt::AlignVCenter, str );
					return;
				}
			case BOTTOM_CENTER:
				{
					const QRect text_rect { rect.bottomLeft() + QPoint( 0, -strip_size ), size };
					painter->drawText( text_rect, Qt::AlignCenter, str );
					return;
				}
			case BOTTOM_RIGHT:
				{
					const QRect text_rect { rect.bottomLeft() + QPoint( -10, -strip_size ), size };
					painter->drawText( text_rect, Qt::AlignRight | Qt::AlignVCenter, str );
					return;
				}
			default:
				break;
		}
	}
}

void RecordBannerDelegate::reloadConfig()
{
	ZoneScoped;
	m_grid_size = { config::grid_ui::gridSizeX::get(), config::grid_ui::gridSizeY::get() };
	m_scale_type = config::grid_ui::imageLayout::get();
	m_strip_height = config::grid_ui::overlayHeight::get();
	m_overlay_opacity = config::grid_ui::overlayOpacity::get();
	m_enable_top_overlay = config::grid_ui::enableTopOverlay::get();
	m_enable_bottom_overlay = config::grid_ui::enableBottomOverlay::get();
	m_feather_radius = config::grid_ui::featherRadius::get();
	m_blur_radius = config::grid_ui::blurRadius::get();
	m_blur_type = config::grid_ui::blurType::get();
	m_enable_capsule_border = config::grid_ui::enableCapsuleBorder::get();
	m_font_size = config::grid_ui::fontSize::get();
	m_font_family = config::grid_ui::font::get();
	m_title_location = config::grid_ui::titleLocation::get();
	m_engine_location = config::grid_ui::engineLocation::get();
	m_version_location = config::grid_ui::versionLocation::get();
	m_creator_location = config::grid_ui::creatorLocation::get();
	m_grid_spacing = config::grid_ui::gridSpacing::get();
}

RecordBannerDelegate::RecordBannerDelegate( QWidget* parent ) :
  QAbstractItemDelegate( parent ),
  m_grid_size { config::grid_ui::gridSizeX::get(), config::grid_ui::gridSizeY::get() },
  m_scale_type { config::grid_ui::imageLayout::get() },
  m_strip_height { config::grid_ui::overlayHeight::get() },
  m_overlay_opacity { config::grid_ui::overlayOpacity::get() },
  m_enable_top_overlay { config::grid_ui::enableTopOverlay::get() },
  m_enable_bottom_overlay { config::grid_ui::enableBottomOverlay::get() },
  m_feather_radius { config::grid_ui::featherRadius::get() },
  m_blur_radius { config::grid_ui::blurRadius::get() },
  m_blur_type { config::grid_ui::blurType::get() },
  m_enable_capsule_border { config::grid_ui::enableCapsuleBorder::get() },
  m_font_size { config::grid_ui::fontSize::get() },
  m_font_family { config::grid_ui::font::get() },
  m_title_location { config::grid_ui::titleLocation::get() },
  m_engine_location { config::grid_ui::engineLocation::get() },
  m_version_location { config::grid_ui::versionLocation::get() },
  m_creator_location { config::grid_ui::creatorLocation::get() },
  m_grid_spacing { config::grid_ui::gridSpacing::get() }
{
	CONFIG_ATTACH_THIS;
}