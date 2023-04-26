//
// Created by kj16609 on 3/2/23.
//

#include "imageManager.hpp"

#include <fstream>

#include <QCryptographicHash>
#include <QFile>
#include <QImageReader>
#include <QImageWriter>
#include <QMessageBox>
#include <QPixmap>

#include <tracy/Tracy.hpp>

#include "atlas/database/Database.hpp"
#include "config.hpp"
#include "system.hpp"

namespace imageManager
{
	void cleanOrphans()
	{
		ZoneScoped;
		spdlog::debug( "Clearing orphan previews/banners" );
		//Grab all images from the database
		Transaction transaction { Transaction::Autocommit };

		for ( const auto& path : std::filesystem::directory_iterator( config::paths::images::getPath() ) )
		{
			if ( !path.is_regular_file() ) continue;

			bool found { false };
			transaction << "SELECT count(*) FROM images WHERE path = ?"
						<< std::filesystem::relative( path, config::paths::images::getPath() ).string()
				>> [ & ]( [[maybe_unused]] int count ) { found = true; };

			if ( !found ) std::filesystem::remove( path );
		}
	}

	std::filesystem::path importImage( const std::filesystem::path& path )
	{
		ZoneScoped;
		if ( std::filesystem::exists( path ) )
		{
			const QString qstr { QString::fromStdString( path.string() ) };
			QImage temp_image;

			if ( !temp_image.load( qstr ) )
			{
				spdlog::error( "Failed to load image {}", path.string() );
				return {};
			}

			//Save the image to a temp file depending on os
			const std::filesystem::path temp_path { std::filesystem::temp_directory_path() / "atlas" / "temp.webp" };

			if ( !std::filesystem::exists( temp_path.parent_path() ) ) create_directories( temp_path.parent_path() );

			std::string image_type { config::images::image_type::get().toStdString() };

			const auto supported_write_types = QImageWriter::supportedImageFormats();
			const auto supported_read_types = QImageReader::supportedImageFormats();

			//Populate list with entires that are in both lists
			std::vector< std::string > supported_types;

			for ( const auto& type : supported_write_types )
			{
				if ( std::find( supported_read_types.begin(), supported_read_types.end(), type )
				     != supported_read_types.end() )
				{
					supported_types.push_back( type.toStdString() );
				}
			}

			//Check if our image type is supported
			if ( std::find( supported_types.begin(), supported_types.end(), image_type ) == supported_types.end() )
			{
				spdlog::warn( "Image type {} is not supported, using png instead", image_type );
				image_type = "png";
				config::images::image_type::set( "png" );
				QMessageBox::warning(
					nullptr,
					"Image Type Not Supported",
					"The image type you have selected is not supported by your system, Image type has been set to png" );
			}

			temp_image.save( QString::fromStdString( temp_path.string() ), image_type.c_str(), 100 );

			const auto hash_file = []( const std::filesystem::path& hash_path ) -> QByteArray
			{
				if ( std::ifstream ifs( hash_path ); ifs )
				{
					std::vector< char > buffer;
					buffer.resize( std::filesystem::file_size( hash_path ) );
					ifs.read( buffer.data(), static_cast< long >( buffer.size() ) );

					QCryptographicHash hash { QCryptographicHash::Sha256 };
					hash.addData( { reinterpret_cast< const char* >( buffer.data() ),
					                static_cast< qsizetype >( buffer.size() ) } );

					return hash.result();
				}
				else
					return {};
			};

			if ( std::filesystem::file_size( temp_path ) > std::filesystem::file_size( path ) )
			{
				//File is worse. So we keep the smaller one.
				std::filesystem::remove( temp_path );

				const auto image_hash { hash_file( path ) };

				const auto dest_path { config::paths::images::getPath()
					                   / ( image_hash.toHex().toStdString() + path.extension().string() ) };

				if ( !std::filesystem::exists( dest_path ) ) std::filesystem::copy( path, dest_path );

				return dest_path;
			}
			else
			{
				//Keep the temp file
				const auto image_hash { hash_file( temp_path ) };

				const auto dest_path { config::paths::images::getPath()
					                   / ( image_hash.toHex().toStdString() + "webp" ) };

				if ( !std::filesystem::exists( dest_path ) ) std::filesystem::copy( temp_path, dest_path );

				std::filesystem::remove( temp_path );

				return dest_path;
			}
		}
		else
			spdlog::warn( "Failed to set open path for new banner at {:ce}", path );

		return { ":/invalid.jpg" };
	}

} // namespace imageManager
