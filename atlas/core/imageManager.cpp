//
// Created by kj16609 on 3/2/23.
//

#include "imageManager.hpp"

#include <QBuffer>
#include <QCryptographicHash>
#include <QFile>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QPixmap>
#include <QtConcurrentRun>

#include <tracy/TracyC.h>

#include <fstream>

#include "config.hpp"
#include "core/database/Transaction.hpp"
#include "core/utils/threading/pools.hpp"

namespace imageManager
{
	void cleanOrphans()
	{
		ZoneScoped;
		spdlog::debug( "Clearing orphan previews/banners" );
		//Grab all images from the database
		RapidTransaction transaction {};

		for ( const auto& path : std::filesystem::directory_iterator( config::paths::images::getPath() ) )
		{
			if ( !path.is_regular_file() ) continue;

			bool found { false };
			transaction << "SELECT count(*) FROM images WHERE path = ?"
						<< std::filesystem::relative( path, config::paths::images::getPath() ).string()
				>> [ & ]( [[maybe_unused]] int count ) noexcept { found = true; };

			if ( !found ) std::filesystem::remove( path );
		}
	}

	std::filesystem::path internalImportImage( const std::filesystem::path& path, const RecordID game_id )
	{
		spdlog::debug( path );
		ZoneScoped;
		if ( !std::filesystem::exists( path ) )
		{
			atlas::logging::userwarn( fmt::format(
				"importImage failed. Attempted to open file {} which doesn't exist anymore. Wrong permissions?",
				path ) );
			throw std::runtime_error( fmt::format( "Filepath {} does not exist. Unable to add as image", path ) );
		}

		//Load file so we have direct access to the bytearray
		QFile file( path );
		if ( !file.open( QFile::ReadOnly ) )
		{
			spdlog::error( "Failed to open image file located at: {}", path );
			throw std::runtime_error( fmt::format( "Failed to load image from file: {}", path ) );
		}
		TracyCZoneN( tracy_ImageLoad, "Image load", true );
		const QByteArray byteArray { file.readAll() };
		TracyCZoneEnd( tracy_ImageLoad );
		file.close();

		const std::string ext { path.extension().string().substr( 1 ) };
		const auto dest_root { config::paths::images::getPath() / std::to_string( game_id ) };
		std::filesystem::create_directories( dest_root );

		TracyCZoneN( tracy_SaveImage, "Image save to buffer as WEBP", true );
		QByteArray webp_byteArray;
		QBuffer webp_buffer( &webp_byteArray );
		temp_image.save( &webp_buffer, "webp", 90 );
		TracyCZoneEnd( tracy_SaveImage );

		const std::string image_type { config::images::image_type::get().toStdString() };

		constexpr std::uint16_t webp_max { 16383 };
		if ( ( temp_image.width() > webp_max ) || ( temp_image.height() > webp_max ) ) // Dimensions too big for WebP?
		{
			spdlog::error( "File is too big for webp" );
		}

		//If GIF then store, do not convert
		if ( ext == "gif" )
		{
			auto dest { getDestFilePath( byteArray, dest_root, path.extension().string() ) };
			//Qt is stupid and will not save gifs...  so we have to copy it
			//const bool file_copied { std::filesystem::copy_file( path, dest ) };
			if ( std::ofstream ofs( dest, std::ios::binary ); ofs )
			{
				ofs.write( byteArray.data(), byteArray.size() );
			}
			else
				throw std::runtime_error( fmt::format( "Unable to save gif to images folder: {}", path.filename() ) );
		}

		//if webp conversion is bigger then save original image
		if ( ( webp_buffer.size() >= byteArray.size() ) ) // Is WebP bigger? Write the other format.
		{
			auto dest { getDestFilePath( byteArray, dest_root, path.extension().string() ) };
			saveImage( byteArray, dest );
			return dest;
		}
		else
		{
			auto dest { getDestFilePath( webp_byteArray, dest_root, ".webp" ) };
			saveImage( webp_byteArray, dest );
			return dest;
		}

		//return dest;
	}

	QFuture< std::filesystem::path > importImage( const std::filesystem::path& path, const RecordID game_id )
	{
		return QtConcurrent::run( &( globalPools().image_importers ), &internalImportImage, path, game_id );
	}

	QByteArray hashData( const char* data_ptr, const int size ) // Hash data with Sha256
	{
		ZoneScopedN( "Hash" );
		QCryptographicHash hash { QCryptographicHash::Sha256 };

		hash.addData( { data_ptr, size } );

		return hash.result();
	}

	std::filesystem::path getDestFilePath(
		QByteArray byteArray,
		std::filesystem::path dest_root,
		std::string ext ) // Use the image hash + ext as its filename
	{
		QByteArray hash { hashData( byteArray, static_cast< int >( byteArray.size() ) ) };
		std::filesystem::path dest { dest_root / ( hash.toHex().toStdString() + ext ) };
		return dest;
	}

	void saveImage( QByteArray byteArray, std::filesystem::path dest )
	{
		const QImage img { QImage::fromData( byteArray ) };
		img.save( QString::fromStdString( dest.string() ) );
	};
} // namespace imageManager
