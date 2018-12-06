#include <algorithm>

int main(int argc, char* argv[])
{
	::std::string str = "mystring";

	// Count character in string
	::std::size_t num = ::std::count( str.begin(), str.end(), '_');

	// Convert string to lower/upper case
	::boost::to_lower_copy( std::string );
	::boost::to_upper_copy( std::string );

	// Trim string
	::boost::trim_left_if( str, ::boost::is_any_of(" \n\r\t"));
	::boost::trim_right_if( str, ::boost::is_any_of(" \n\r\t"));

	// Split string depending on character
	::std::string str = "y.o.u_r"
	::std::vector< ::std::string > elements;
	::boost::split(elements, str, ::boost::is_any_of("_."));

  // Check if string start/ends with characters
	if( ::boost::algorithm::starts_with( str, "###" ) && ::boost::algorithm::ends_with( str, "---") )
	{
		// string starts with "###" and ends with "---"
	}
	return 0;
}

