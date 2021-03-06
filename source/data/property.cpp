#include <hcube/data/property.h>

namespace hcube
{
	properties_vector::properties_vector(std::initializer_list< property* > properties)
	{
		//alloc
		m_properties.reserve(properties.size());
		//push all
		for (property* p : properties) m_properties.push_back(p);
	}

	properties_vector::properties_vector(
		const std::vector< const properties_vector* >& extends,
		const std::vector< property* >& properties
	)
	{
		//for all class
		for (const properties_vector* ex_properties : extends)
		//for all properties
		for (property* p : *ex_properties)
		//add
		m_properties.push_back(p);
		//push all properties of this class
		for (property* p : properties) m_properties.push_back(p);
		
	}


	properties_vector::~properties_vector()
	{
		for (property* p : m_properties) delete p;
	}

	//iterators
	properties_vector::property_list_const_it properties_vector::begin() const
	{
		return m_properties.begin();
	}

	properties_vector::property_list_const_it properties_vector::end() const
	{
		return m_properties.end();
	}
}