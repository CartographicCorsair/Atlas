//
// Created by kj16609 on 3/15/23.
//

#include "RecordListModel.hpp"

#include <moc_RecordListModel.cpp>

void RecordListModel::setRecords( std::vector< Record > records )
{
	beginResetModel();
	m_records = std::move( records );
	endResetModel();
	emit recordsChanged( m_records );
}

void RecordListModel::addRecord( Record record, const std::size_t place_at )
{
	const int pos { static_cast< int >( std::min( place_at, m_records.size() ) ) };
	beginInsertRows( {}, pos, pos );
	m_records.insert( m_records.begin() + static_cast< int >( pos ), record );
	endInsertRows();
	emit recordsChanged( m_records );
}

void RecordListModel::removeRecord( QPersistentModelIndex index )
{
	if ( !index.isValid() )
		throw std::runtime_error( "RecordListModel::removeRecord(QPersistentModelIndex): index is not valid" );

	beginRemoveRows( {}, index.row(), index.row() );
	auto itter { m_records.begin() + index.row() };
	m_records.erase( itter );
	endRemoveRows();
	emit recordsChanged( m_records );
}

int RecordListModel::rowCount( [[maybe_unused]] const QModelIndex& index ) const
{
	return static_cast< int >( m_records.size() );
}

QVariant RecordListModel::data( const QModelIndex& index, int role ) const
{
	switch ( role )
	{
		case Qt::DisplayRole:
			return QVariant::fromStdVariant( std::variant<
											 Record >( m_records.at( static_cast< std::size_t >( index.row() ) ) ) );
		default:
			return {};
	}
}
