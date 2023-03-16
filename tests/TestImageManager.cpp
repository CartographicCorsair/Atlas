//
// Created by kj16609 on 3/2/23.
//

#include <QApplication>
#include <QPixmap>

#include "GTestBox.hpp"
#include "h95/database/Database.hpp"
#include "h95/imageManager.hpp"

class TestImageManager : public ::testing::Test
{
	int argc { 0 };
	char** argv { nullptr };

	QApplication* app { nullptr };

	void SetUp() override
	{
		app = new QApplication( argc, argv );
		Database::initalize( "./data/testing.db" );
	}

	void TearDown() override
	{
		delete app;
		Database::deinit();
		std::filesystem::remove_all( "./data/" );
	}
};

TEST_F( TestImageManager, importPreview )
{
	QImage image { ":/banner/placeholder.jpg" };
	image.save( "./assets/banner/placeholder.jpg" );

	const auto output { imageManager::importImage( "./assets/banner/placeholder.jpg" ) };

	GTEST_ASSERT_TRUE(
		std::filesystem::canonical(
			"./data/images/de4fb797c8dabce6c9ee87e7e93d3cc5393e5ff4afe6c85634117cb2128feba7.webp" )
		== output );

	GTEST_ASSERT_TRUE( std::filesystem::exists(
		"./data/images/de4fb797c8dabce6c9ee87e7e93d3cc5393e5ff4afe6c85634117cb2128feba7.webp" ) );
}

TEST_F( TestImageManager, importNonExistant )
{
	GTEST_ASSERT_EQ( imageManager::importImage( "./my/fake/image.jpg" ), ":/invalid.jpg" );
}

TEST_F( TestImageManager, clearOrhpans )
{
	QImage image { ":/banner/placeholder.jpg" };
	image.save( "./data/de4fb797c8dabce6c9ee87e7e93d3cc5393e5ff4afe6c85634117cb2128feba7.webp" );

	imageManager::cleanOrphans();

	GTEST_ASSERT_FALSE(
		std::filesystem::exists( "./data/de4fb797c8dabce6c9ee87e7e93d3cc5393e5ff4afe6c85634117cb2128feba7.webp" ) );
}
