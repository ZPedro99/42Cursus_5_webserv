/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lde-sous <lde-sous@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/06 14:04:14 by lde-sous          #+#    #+#             */
/*   Updated: 2024/03/06 14:04:14 by lde-sous         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../headers/server/Server.hpp"

extern volatile sig_atomic_t gSignalStatus;
extern bool chunky;
bool firstChunk = true;

/* ===================== Orthodox Canonical Form ===================== */

Request::Request() : _method(""), _uri(""), _httpVersion(""),
_firstLine(""), _fullRequest(""), _isPageOK(true), _isChunked(false), _isRequestComplete(false)
{
}

Request::Request(const Request& original)
{
	_method = original._method;
	_uri = original._uri;
	_httpVersion = original._httpVersion;
	_firstLine = original._firstLine;
	_fullRequest = original._fullRequest;
	_isChunked = original._isChunked;
	_isRequestComplete = original._isRequestComplete;
	_isPageOK = original._isPageOK;
}

Request& Request::operator=(const Request& original)
{
	if (this != &original)
	{
		_method = original._method;
		_uri = original._uri;
		_httpVersion = original._httpVersion;
		_firstLine = original._firstLine;
		_fullRequest = original._fullRequest;
		_isChunked = original._isChunked;
		_isRequestComplete = original._isRequestComplete;
		_isPageOK = original._isPageOK;
	}
	return (*this);
}

Request::~Request()
{
}

/* ===================== Setter Functions ===================== */

/**
 * @brief Fills the request header by reading data from the socket.
 *
 * This function reads data from the provided socket and fills the request header
 * until a '\r\n' sequence is encountered, indicating the end of the header.
 * It appends the received data to the _fullRequest member variable and extracts
 * the first line of the request to the _firstLine member variable.
 * Once the full request header is received, it calls the parseRequest method
 * to parse the complete request.
 *
 * @param socket The socket descriptor from which to read data.
 */

int		Request::fillHeader(int socket)
{
	char buffer[1024];
	bool firstLine = false;
	while(1) 
	{
		ssize_t bytesRead = recv(socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
		if (bytesRead <= 0)
			break;
		buffer[bytesRead] = 0;
		char *pos = strstr(buffer, "\r\n");
		if (firstLine == false) 
		{
			// This cast prevents basic_string::append error, in case buffer isn't correctly formatted
			size_t len = pos - buffer;   
			std::copy(buffer, buffer + len, std::back_inserter(_firstLine));
			firstLine = true;
		}
		std::copy(buffer, buffer + bytesRead, std::back_inserter(_fullRequest));

		// End case for the last chunk in a chunked request
		if (chunky && strstr(_fullRequest.c_str(), "\r\n0\r\n\r\n")) 
		{
			parseRequest();
			reqLogger(gfullRequest);
			chunky = false;
			_isRequestComplete = true;
			chunkDecoder();
			firstChunk = true;
			return 1;
		}
	}
	if (!chunky)
	{
		if (trimValue("Transfer-Encoding") == "chunked")
			chunky = true;
		else if (trimValue("Content-Length").empty() && _method == "POST")
			chunky = false;
	}
	if (!_fullRequest.empty())
	{
		parseRequest();
		if (chunky)
		{		
			gfullRequest += _fullRequest;
			return 1;
		}
			gfullRequest += _fullRequest;
		reqLogger(gfullRequest);
		firstChunk = true;
	}
	return 0;
}
/* ===================== Getter Functions ===================== */

std::string	Request::getMethod() const
{
	return (_method);
}

std::string	Request::getURI() const
{
	return(_uri);
}

std::string	Request::getHTTPVersion() const
{
	return (_httpVersion);
}

std::string	Request::getContentLen() const
{
	return (_contentLength);
}

std::string	Request::getContentType() const
{
	return (_contentType);
}

std::string	Request::getFilename() const
{
	return(_file);
}

std::string	Request::getBody() const
{
	return (_requestBody);
}

std::string	Request::getHost() const
{
  return (_host);
}

/**
 * @brief Extracts and returns the value of a specified header from an HTTP request string.
 *
 * @param request    The HTTP request string.
 * @param headerName The name of the header to extract the value for.
 * @return The value of the specified header, or an empty string if the header is not found.
 */
std::string Request::getHeader(const std::string& headerName)
{
	std::istringstream requestStream(_fullRequest);
	std::string line;
	while (std::getline(requestStream, line))
	{
		std::istringstream lineStream(line);
		std::string name;
		std::getline(lineStream, name, ':');
		if (name == headerName)
		{
			std::string value;
			std::getline(lineStream, value);
			return value;
		}
	}
	return ("");
}

/* ===================== Parsing Functions ===================== */

/**
 * @brief Parses the full request to extract relevant information.
 *
 * This function parses the full request to extract information such as content type, content length,
 * filename, and request body. It first clears unnecessary characters from the values obtained from
 * the request headers using the trimValue function. Then, it parses the content type to extract
 * the content value and boundary using the parseContentType function. The filename is extracted from
 * the content disposition header using the parseFilename function. Finally, the request body is
 * extracted from the full request using the bodyParser function.
 */
void	Request::parseRequest()
{
	if ((chunky && firstChunk) || firstChunk)
	{
		_contentType = trimValue("Content-Type");
		parseContentType(trimValue("Content-Type"));
		_contentLength = trimValue("Content-Length");
		_file = parseFilename(trimValue("Content-Disposition"));
		_requestBody = bodyParser();
		firstChunk = false;
	}
	else
		gfullRequest += _fullRequest;
}

/**
 * @brief Clears unnecessary characters from the provided string.
 *
 * This function removes extra spaces and newline characters from the beginning and end of the string.
 * It is primarily used to clean up values obtained from request headers.
 *
 * @param toBeTrimmed The string to be trimmed.
 * @return The trimmed string.
 */
std::string Request::trimValue(std::string toBeTrimmed)
{
	std::string clearString = getHeader(toBeTrimmed);
	clearString.erase(0, 1);
	clearString.erase(std::remove(clearString.begin(), clearString.end(), '\r'), clearString.end());
    clearString.erase(std::remove(clearString.begin(), clearString.end(), '\n'), clearString.end());
	return clearString;
}

/**
 * @brief Parses the content type header to extract content value and boundary.
 *
 * This function extracts the content value and boundary from the content type header string.
 * It then stores these values in the corresponding member variables (_contentValue and _boundary).
 *
 * @param ContentType The content type header string.
 */
void Request::parseContentType(std::string ContentType)
{
	size_t dotPos = ContentType.find(";");
	if(dotPos != std::string::npos)
	{
		_contentValue= ContentType.substr(0, dotPos);
		std::string boundary = ContentType.substr(dotPos + 2, ContentType.length());
		size_t boundaryPos = boundary.find("=");
		_boundary = boundary.substr(boundaryPos + 1, boundary.length());
		_boundary.erase(std::remove(_boundary.begin(), _boundary.end(), '\r'), _boundary.end());
        _boundary.erase(std::remove(_boundary.begin(), _boundary.end(), '\n'), _boundary.end());
	}
}

/**
 * @brief Parses the filename from the content disposition header.
 *
 * This function extracts the filename from the content disposition header string.
 * It then removes surrounding quotes if they exist and returns the extracted filename.
 *
 * @param ClearDisposition The content disposition header string.
 * @return The extracted filename.
 */
std::string Request::parseFilename(std::string ClearDisposition)
{
	std::string needle = "filename=";
	std::string filename;
    size_t startPos = ClearDisposition.find(needle);

	if (startPos != std::string::npos)
	{
		startPos += needle.length();
		filename = ClearDisposition.substr(startPos, ClearDisposition.length() - startPos);
        if (filename[0] == '\"') 
			filename.erase(0, 1);
        if (filename[filename.length() - 1] == '\"')
			filename.erase(filename.length() - 1, 1);
	}
	return filename;
}

/**
 * @brief Extracts the request body from the full request.
 *
 * This function extracts the request body from the full request by finding the start and end markers.
 * It then returns the extracted body content.
 *
 * @return The extracted request body.
 */
std::string Request::bodyParser()
{
	if (!chunky)
	{
		bool final = false;
		std::string finalBoundary = "--" + _boundary + "--";
		std::string body;
		std::string marker = "Content-Disposition:";
		std::string::size_type startPos = _fullRequest.find(marker);
		if (startPos == std::string::npos)
			return "";
		startPos = _fullRequest.find("\r\n\r\n", startPos);
		if (startPos == std::string::npos)
		{
			std::cout << "Start of content not found." << std::endl;
			return "";
		}
		startPos += 4;
		std::string::size_type endPos = _fullRequest.find(finalBoundary, startPos);
		if (endPos == std::string::npos)
			final = true;
		body = _fullRequest.substr(startPos, endPos - startPos);
		if (final)
			body.append(finalBoundary);
		return body;
	}
	else
	{
		std::string body;
		std::string::size_type pos = _fullRequest.find("\r\n\r\n");
		body = _fullRequest.substr(pos, std::string::npos - pos);
    	return body;
	}
}


/* ===================== Request Attribute Functions ===================== */

/**
 * @brief Validates the request method against the server configuration.
 *
 * This function validates whether the request method is allowed for the requested URI based on the server's configuration.
 * It checks if the method is allowed for the current URI location or its parent directories. If allowed, it returns true; otherwise, false.
 * If the requested URI is the root ("/"), it checks if the method is allowed globally by the server.
 * 
 * @param server Pointer to the Server object containing the server configuration.
 * @return true if the request method is allowed, false otherwise.
 */
bool Request::checkMethod(Server* server)
{
	std::vector<BaseLocation*>::const_iterator it;
	std::string currentURI;
	if (server->getConf().locationStruct.empty())
	{
		if (currentURI == "root" || _uri == "/")
		{
			if ((_method == "GET" && server->isGETAllowed()) ||
				(_method == "POST" && server->isPOSTAllowed()) ||
				(_method == "DELETE" && server->isDELETEAllowed()))
					return true;
			return false;
		}
	}
	for (it = server->getConf().locationStruct.begin(); it != server->getConf().locationStruct.end(); ++it)
	{
		try
		{
			LocationDir* dir = dynamic_cast<LocationDir*>(*it);
			if (dir && dir->name == _uri)
			{
				currentURI = _uri;
				if ((_method == "GET" && std::find(dir->allow_methods.begin(), dir->allow_methods.end(), "GET") != dir->allow_methods.end()) ||
					(_method == "POST" && std::find(dir->allow_methods.begin(), dir->allow_methods.end(), "POST") != dir->allow_methods.end()) ||
					(_method == "DELETE" && std::find(dir->allow_methods.begin(), dir->allow_methods.end(), "DELETE") != dir->allow_methods.end()))
						return true;
			}
			else if (dir)
			{
				std::vector<LocationFiles*>::iterator file_it = dir->files.begin();
				for(; file_it != dir->files.end(); ++file_it)
				{
					LocationFiles* file = *file_it;
                    size_t extPos = _uri.rfind(".");
                    std::string fileExt;
                    if (extPos != std::string::npos)
                    	fileExt =  _uri.substr(extPos, _uri.length() - 1);
                    else
                    	continue ;
					if (file && (file->name.find(fileExt)) != std::string::npos)
					{
						currentURI = _uri;
						if ((_method == "GET" && std::find(file->allow_methods.begin(), file->allow_methods.end(), "GET") != file->allow_methods.end()) ||
							(_method == "POST" && std::find(file->allow_methods.begin(), file->allow_methods.end(), "POST") != file->allow_methods.end()) ||
							(_method == "DELETE" && std::find(file->allow_methods.begin(), file->allow_methods.end(), "DELETE") != file->allow_methods.end()))
								return true;
					}
				}
			}
			else
			{
				LocationFiles* file = dynamic_cast<LocationFiles*>(*it);
				size_t extPos = _uri.rfind(".");
				std::string fileExt;
				if (extPos != std::string::npos)
					fileExt =  _uri.substr(extPos, _uri.length() - 1);
				else
					continue ;
				if (file && (file->name.find(fileExt) != std::string::npos))
				{
					currentURI = _uri;
					if ((_method == "GET" && std::find(file->allow_methods.begin(), file->allow_methods.end(), "GET") != file->allow_methods.end()) ||
						(_method == "POST" && std::find(file->allow_methods.begin(), file->allow_methods.end(), "POST") != file->allow_methods.end()) ||
						(_method == "DELETE" && std::find(file->allow_methods.begin(), file->allow_methods.end(), "DELETE") != file->allow_methods.end()))
							return true;
				}
				else
					currentURI = "root";
			}
			if (currentURI == "root" || _uri == "/")
			{
				if ((_method == "GET" && server->isGETAllowed()) ||
					(_method == "POST" && server->isPOSTAllowed()) ||
					(_method == "DELETE" && server->isDELETEAllowed()))
						return true;
			}
		}
		catch(std::bad_cast &e)
		{
			std::cout << "Failed Casting on Request::checkMethod" << std::endl;
		}
	}
	_isPageOK = false;
	return false;
}

/**
 * @brief Parses the attributes of the HTTP request.
 *
 * This function parses the method, URI, and HTTP version from the provided request string.
 * It also removes trailing slash from the URI if present and checks for basic request validity.
 * 
 * @param request The HTTP request string to parse.
 * @return 0 if parsing is successful, otherwise returns an error code (403 for forbidden or 400 for bad request).
 */
int Request::fillAttributes(const std::string& request) {
	std::istringstream iss(request);
	iss >> _method;
	iss >> _uri;
	iss >> _httpVersion;
	if (_uri.at(_uri.length() - 1) == '/' && _uri.length() != 1)
		_uri.erase(_uri.length() - 1);
	if (_uri.empty() || _method.empty() || (_httpVersion.empty() || _httpVersion != "HTTP/1.1"))
		return (403);
	if (_method != "GET" && _method != "POST" && _method != "DELETE")
		return (400);
	std::string header;
	std::istringstream iss2(_fullRequest);
	while (iss2 >> header && header != "Host:")
	{
	}
	if (iss2 >> _host)
		return (0);
	return (403);
}

/**
 * @brief Parses and validates the incoming request.
 *
 * This function parses the incoming HTTP request, validates its attributes, and checks if the requested method is allowed.
 * If any parsing or validation error occurs, it returns the appropriate HTTP error code (403 for forbidden or 400 for bad request).
 * If the request method is not allowed, it returns 405 (Method Not Allowed). Otherwise, it returns 200 (OK) indicating successful parsing and validation.
 * 
 * @param server Pointer to the Server object containing the server configuration.
 * @return The HTTP response code indicating the status of request parsing and validation.
 */
int	Request::ConfigureRequest(Server* server)
{
	try
	{
		int code = 0;
		if (_firstLine.empty() && !gfullRequest.empty())
		{
			code = fillAttributes(gfullRequest);
			if (code != 0)
				return (code);	
		}
		else
		{
			code = fillAttributes(_firstLine);
			if (code != 0)
				return (code);
		}
		if (!checkMethod(server))
		{
			if (_isPageOK == false)
				return (404);
			return (405);
		}
	}
	catch (std::exception &e)
	{
		std::cout << "Cant Parse Request" << std::endl;
	}
	return (200);
}

/**
 * @brief Checks if the size of the request body exceeds the server's maximum allowed size.
 *
 * This function calculates the size of the request body using getContentLength() and compares it with the maximum allowed size specified in the server's configuration.
 * If the request body size exceeds the maximum allowed size, it returns 413 (Payload Too Large); otherwise, it returns 0 indicating no size limit violation.
 * 
 * @param server Pointer to the Server object containing the server configuration.
 * @return The HTTP response code indicating the status of the request body size check.
 */
int Request::checkClientBodySize(Server* server)
{
	double sizeInMB = getContentLength();
	unsigned int maxBodySize = server->getConf().client_max_body_size;
	if (sizeInMB > maxBodySize)
		return 413;
	return 0;
}

/**
 * @brief Calculates the size of the request body in megabytes.
 *
 * This function calculates the size of the requfillRequestHeaderest body in megabytes using the length of the full request string obtained from getContentLen().
 * It converts the length from bytes to megabytes and returns the result.
 * If the full request string is empty, it throws a RequestException.
 * 
 * @return The size of the request body in megabytes.
 * @throws RequestException if the full request string is empty.
 */
double	Request::getContentLength()
{
	if (!_fullRequest.empty())
	{
		size_t lengthInBytes = std::atoi(getContentLen().c_str());
		double sizeInMB = static_cast<double>(lengthInBytes)  / (1024.0 * 1024.0);
		return sizeInMB;
	}
	else
		throw RequestException("");
	return 0;
}


/* ===================== Logger Functions ===================== */

/**
 * @brief Logs an HTTP request to a file along with a timestamp.
 *
 * @param request The HTTP request string to log.
 * @throw RequestException If an error occurs while creating or writing to the log file.
 */
void	Request::reqLogger(std::string request)
{
	std::string logs = request;
	if (createDirectory("./logs/requests"))
	{
		std::fstream outfile("./logs/requests/requestlogs.log", std::ios_base::app);
		std::time_t timestamp = std::time(NULL);
		char buff[50];
		std::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
		if (!outfile.fail())
		{
			std::replace(logs.begin(), logs.end(), '\r', ' ');
			outfile << std::endl;
			outfile << "====================== " << buff << " =======================" << std::endl;
			outfile << logs << std::endl;
		}
		outfile.close();
	}
	else throw RequestException("Failed to create request logs.");
}

/* ===================== Exceptions ===================== */

/**
 * @brief Constructs a RequestException object with the given error message.
 *
 * @param error The error message to include in the exception.
 */
Request::RequestException::RequestException(const std::string& error)
{
	if (!error.empty())
	{
		std::ostringstream err;
		err << BOLD << RED << "Error: " << error << RESET;
		_errMessage = err.str();
	}
}

Request::RequestException::~RequestException() throw()
{
}

/**
 * @brief Returns the error message associated with this exception.
 *
 * @return The error message as a C-style string.
 */
const char *Request::RequestException::what() const throw()
{
	return (_errMessage.c_str());
}

/**
 * @brief Unchunks the chunked request previsouly received.
 * 
 * This function parses the data stored in the gfullRequest variable and outputs it.
 * 
 */
void	Request::chunkDecoder()
{
	size_t pos = gfullRequest.find("\r\n\r\n");
	if (pos == std::string::npos)
		return ;
	std::istringstream chunks(gfullRequest.substr(pos + 4));
	std::string	line;
	size_t size = 0;
	int flag = 1;
	while (getline(chunks, line))
	{
		if (flag % 2 != 0)
		{
			std::istringstream lineSize(line);
			lineSize >> std::hex >> size;
		}
		else
		{
			std::string data = line.substr(0, size);
			if (data.length() == size)
				_requestBody += data;
		}
		flag++;
	}
}