/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lde-sous <lde-sous@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/05 02:17:25 by lde-sous          #+#    #+#             */
/*   Updated: 2024/04/05 02:17:25 by lde-sous         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../headers/server/Server.hpp"
std::string gfullRequest;
/* ===================== Orthodox Canonical Form ===================== */

Server::Server() {}

Server::Server(const Server& original) 
{
	(void)original;
}

Server& Server::operator=(const Server& original) 
{
	(void)original;
	return (*this);
}

Server::~Server() {}

/* ===================== Constructors ===================== */

Server::Server(const t_listen& listen) 
{
	_listen.port = listen.port;
	_listen.host = listen.host;
	_serverConfig.server_root = "./var/www/html/";
	_serverConfig.index.push_back("index.htm");
	_serverConfig.index.push_back("index.html");
	_serverConfig.index.push_back("index.php");
	_isServerOn = false;
	_envp.auth_mode = "AUTH_MODE=";
	_envp.server_port = "SERVER_PORT=" + intToStr(_listen.port);
	_isCGI = false;
}

/* ===================== Getter Functions ===================== */

long Server::getFD() 
{
	return _socketfd;
}

std::vector<Connection>&	Server::getOpenConnections() 
{
	return (_connectionsVector);
}

Connection&		Server::getConnection(int toFind) 
{
	std::vector<Connection>::iterator it;
	for (it = _connectionsVector.begin(); it != _connectionsVector.end(); ++it) 
	{
		if (it->getConnectionFD() == toFind)
			break ;
	}
	return (*it);
}

std::vector<Connection>& Server::getConnectionVector() 
{
	return (_connectionsVector);
}

const std::vector<std::string>& Server::getBody() const 
{
	return (_body);
}

std::vector<std::string>& Server::getDynamicBody() 
{
	return (_body);
}

const t_server_config&	Server::getConf() const 
{
	return (_serverConfig);
}

t_server_config&	Server::getDynamicConfig() 
{
	return (_serverConfig);
}

const t_listen&	Server::getListen() const 
{
	return (_listen);
}

t_listen&	Server::getDynamicListen() 
{
	return (_listen);
}

sockaddr_in*	Server::getSockaddr() 
{
	return (&_sockaddr);
}

bool	Server::getServerStatus() const 
{
	return (_isServerOn);
}

bool	Server::isGETAllowed() const 
{
	return (_GETAllowed);
}

bool	Server::isPOSTAllowed() const 
{
	return (_POSTAllowed);
}

bool	Server::isDELETEAllowed() const 
{
	return (_DELETEAllowed);
}

/* ===================== Setter Functions ===================== */

/**
 * @brief Sets the file descriptor of the server.
 *
 * This function sets the file descriptor of the server to the specified value.
 *
 * @param fd The file descriptor to set.
 */
void	Server::setFD(long fd) 
{
	_socketfd = fd;
}

/**
 * @brief Sets the address of the server.
 *
 * This function configures the address family, port, and IP address of the server.
 * It initializes the sockaddr_in structure with the appropriate values.
 */
void	Server::setAddr() 
{
	_sockaddr.sin_family = AF_INET; // Identifier for sockaddr_in by default
	_sockaddr.sin_port = htons(_listen.port);   // listen for t_listen _listen.port //
	_sockaddr.sin_addr.s_addr = htonl(_listen.host) ; /*listen for t_listen _listen.host*/
}

/**
 * @brief Sets the connection descriptor of the server.
 *
 * This function sets the connection descriptor of the server by adding a new connection
 * descriptor to the list of connections.
 *
 * @param connection The connection descriptor to set.
 */
void	Server::setConnection(int connection) 
{
	_connectionsVector.push_back(Connection(connection));
}

/* ===================== Setup Functions ===================== */

/**
 * @brief Sets up the server to start accepting connections.
 *
 * This function performs the necessary steps to set up the server for accepting connections.
 * It creates a socket, binds it to the specified address and port, and starts listening for connections.
 * Additionally, it initializes the server's allowed HTTP methods based on the server configuration.
 * If any step fails, it throws a ServerException with an appropriate error message.
 */
void Server::initServer() 
{

	std::ostringstream errorMsg;
	// Create socket
	setFD(socket(AF_INET, SOCK_STREAM, 0));
	if (getFD() == -1)
		throw ServerException("Server Creation: Could not create socket.");

	// Allow to rebind ports that have TIME_WAIT status
	int optval = 1;
	if (setsockopt(getFD(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		throw ServerException("Server Creation: Could not set SO_REUSEADDR.");

	// Bind address hosts to the socket
	setAddr();
	if (bind(getFD(), reinterpret_cast<const sockaddr*>(&_sockaddr), sizeof(_sockaddr)) == -1)
		throw ServerException("Server Creation: Could not bind port.");

	// Start listening for connections
	if (listen(getFD(), 20) < 0)
		throw ServerException("Server Creation: Could not listen on socket.");

	// Initialize server allowed methods
	_GETAllowed = std::find(_serverConfig.allow_methods.begin(), _serverConfig.allow_methods.end(), "GET") != _serverConfig.allow_methods.end();
	_POSTAllowed = std::find(_serverConfig.allow_methods.begin(), _serverConfig.allow_methods.end(), "POST") != _serverConfig.allow_methods.end();
	_DELETEAllowed = std::find(_serverConfig.allow_methods.begin(), _serverConfig.allow_methods.end(), "DELETE") != _serverConfig.allow_methods.end();
	_isServerOn = true;
}

/**
 * @brief Sets the specified socket to non-blocking mode.
 *
 * This function sets the specified socket to non-blocking mode by modifying its socket flags.
 * If the operation fails, it throws a ServerException with an appropriate error message.
 *
 * @param socket The socket descriptor to set to non-blocking mode.
 */
void Server::setNonBlock(int socket) 
{

	// Set the modified socket flags
	if (fcntl(socket, F_SETFL, O_NONBLOCK) == -1)
		throw ServerException("Can't set connection non-block flags");
}

/* ===================== CGI Execution Functions ===================== */

/**
 * @brief Tests if the request method is DELETE and executes the corresponding CGI script if found.
 *
 * This function checks if the request method is DELETE and attempts to execute the corresponding CGI script.
 * If the URI matches the configured location and file extension, it executes the CGI script using the specified script path.
 * After execution, it sends the appropriate response back to the client.
 *
 * @param uri The URI from the request.
 * @param fd The file descriptor of the client connection socket.
 * @param req The request object.
 * @param resp The response object.
 */
void	Server::deleteCGI(const std::string& uri, int fd, Request& req, Response& resp) 
{

	if(req.getMethod() == "DELETE")
	{
		std::vector<BaseLocation*>::iterator it = _serverConfig.locationStruct.begin();
		for (; it != _serverConfig.locationStruct.end(); ++it) 
		{
			LocationFiles* file = dynamic_cast<LocationFiles*>(*it);
			// Get the uri extension for the cgi file
			size_t extPos = uri.rfind(".");
			std::string fileExt;
			if (extPos != std::string::npos) 
			{
				fileExt = uri.substr(extPos, uri.length() - 1);

				// Check if the file is the same type as uri
				if (file && (file->name.find(fileExt) != std::string::npos)) 
				{
					executeDeleteCGIScript("." + file->cgi_pass, req, fd, resp);
					_isCGI = true;
					return ;
				}
			}
		}
		_isCGI = false;
	}
}

/**
 * @brief Tests if the request method is POST and executes the corresponding CGI script if found.
 *
 * This function checks if the request method is POST and attempts to execute the corresponding CGI script.
 * If the URI matches the configured location and file extension, it executes the CGI script using the specified script path.
 * After execution, it sends the appropriate response back to the client.
 *
 * @param uri The URI from the request.
 * @param fd The file descriptor of the client connection socket.
 * @param req The request object.
 * @param resp The response object.
 * @param reqCode Reference to the request code.
 * @return An integer representing the status code.
 */
int	Server::postCGI(const std::string& uri, int fd, Request& req, Response& resp, int& reqCode) 
{
	// Checking if request is POST
	if (req.getMethod() == "POST")
	{
		if (req.getFilename() == "") 
		{
			if (req.getContentLen() == "0" || req.getBody().size() == 2) 
			{
				reqCode = 204;
				return (reqCode);
			}
			reqCode = 200;
			return (reqCode);
		}
		std::string contentlen(req.getHeader("Content-Length"));
		envCGI(_serverConfig, uri, _envp, req);
		if (std::atoi(contentlen.c_str()) == 0) 
		{
			std::cout << "No content was sent" << std::endl;
			reqCode = 204;
		}
		else 
		{
			// Checking if the location is a file on root
			std::vector<BaseLocation *>::iterator it = _serverConfig.locationStruct.begin();
			for (; it != _serverConfig.locationStruct.end(); ++it) 
			{
				LocationFiles *file = dynamic_cast<LocationFiles *>(*it);

				// Get the uri extension for the cgi file
				size_t extPos = uri.rfind(".");
				std::string fileExt;
				if (extPos != std::string::npos)
					fileExt = uri.substr(extPos, uri.length() - 1);
				else
					break;
				// Check if the file is the same type as uri
				if (file && (file->name.find(fileExt) != std::string::npos) && file->cgi_pass.find(uri) != std::string::npos) 
				{
					std::string method = _envp.request_method.substr(_envp.request_method.find_first_of('=') + 1);
					if (std::find(file->allow_methods.begin(), file->allow_methods.end(), method) != file->allow_methods.end()) 
					{
						executeCGIScript("." + file->cgi_pass, req, fd,	resp);
						_isCGI = true;
						return 0;
					}
					return 405;
				}

				// If it's not on root level, check every subdirectory location level to see if the file is inside it
				else 
				{
					LocationDir *dir = dynamic_cast<LocationDir *>(*it);
					if (dir) {
						std::vector<LocationFiles *>::iterator file_it = dir->files.begin();
						for (; file_it != dir->files.end(); ++file_it) 
						{
							LocationFiles *file = *file_it;

							// Get the uri extension for the cgi file
							extPos = uri.rfind(".");
							if (extPos != std::string::npos)
								fileExt = uri.substr(extPos, uri.length() - 1);
							else
								break;

							// Check if the file is the same type as uri
							if (file && (file->name.find(fileExt) != std::string::npos) && file->cgi_pass.find(uri) != std::string::npos) 
							{
								size_t posss = _envp.request_method.find_first_of('=') + 1;
								std::string method = _envp.request_method.substr(posss);
								if (std::find(file->allow_methods.begin(), file->allow_methods.end(), method) != file->allow_methods.end()) 
								{
									executeCGIScript("." + file->cgi_pass, req, fd,	resp);
									_isCGI = true;
									return 0;
								}
								return 405;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

/**
 * @brief Tests if the request method is GET and executes the corresponding CGI script if found.
 *
 * This function checks if the request method is GET and attempts to execute the corresponding CGI script.
 * If the URI matches the configured location and file extension, it executes the CGI script using the specified script path.
 * After execution, it sends the appropriate response back to the client.
 *
 * @param uri The URI from the request.
 * @param fd The file descriptor of the client connection socket.
 * @param req The request object.
 * @param resp The response object.
 * @return An integer representing the status code.
 */
int Server::getCGI(const std::string& uri, int fd, Request& req, Response& resp) 
{

	// Finding Query Delimiter
	size_t QueryDelim = uri.find("?");
	if (QueryDelim != std::string::npos && (uri.find_first_of('?') == uri.find_last_of('?'))) 
	{
		_envp.content_length = "CONTENT_LENGTH=" + req.getContentLen();
		_envp.content_type = "CONTENT_TYPE=" + req.getContentType();
		_envp.gateway_interface = "GATEWAY_INTERFACE=CGI/1.1";
		_envp.query_string = "QUERY_STRING=" + uri.substr(QueryDelim + 1, std::string::npos);
		//PATH INFO
		size_t pos = uri.find(".py");
		std::string script;
		if (pos != std::string::npos && pos < QueryDelim) 
		{
			pos += 3;
			script = "." + uri.substr(0, pos);
		}
		else
			return 404;
		DIR* dir = opendir(script.c_str());
		while (dir != NULL) 
		{
			closedir(dir);
			size_t new_pos = uri.find(".py", pos) + 3;
			if (new_pos != std::string::npos)
				std::copy(uri.begin() + pos, uri.begin() + new_pos, std::back_inserter(script));
			else
				std::copy(uri.begin() + pos, uri.begin() + pos + QueryDelim, std::back_inserter(script));
			pos = new_pos;
			dir = opendir(script.c_str());
		}
		_envp.script_name = "SCRIPT_NAME=" + script;
		if (_envp.script_name.empty())
			return 404;
		script.erase(0, 1);
		pos = uri.find(script);
		if (pos != std::string::npos) 
		{
			std::string aux(uri);
			aux.erase(QueryDelim);
			_envp.path_info = "PATH_INFO=" + aux.substr(pos + script.length());
			pos = _envp.script_name.rfind('/');
			if (pos != std::string::npos) 
			{
				_envp.path_translated = "PATH_TRANSLATED=" + _envp.script_name.substr(_envp.script_name.find_first_of('=') + 1, _envp.script_name.length() - _envp.script_name.rfind('/'));
				_envp.path_translated = _envp.path_translated + _envp.path_info.substr(_envp.path_info.find_first_of('=') + 1);
			}
			else
				_envp.path_translated = "PATH_TRANSLATED=";
		}
		_envp.remote_addr = "REMOTE_ADDR=" + req.trimValue("Remote-Addr");
		_envp.remote_host = "REMOTE_HOST=";
		_envp.remote_ident = "REMOTE_IDENT=";
		_envp.remote_user = "REMOTE_USER=";
		_envp.request_method = "REQUEST_METHOD=" + req.getMethod();
		_envp.server_name = "SERVER_NAME=" + _serverConfig.server_name.back();
		_envp.server_protocol = "SERVER_PROTOCOL=" + req.getHTTPVersion();
		_envp.server_software = "SERVER_SOFTWARE=";


		script = _envp.script_name.substr(pos);

		pos = script.find(".py");
		if (pos != std::string::npos) 
		{
			std::string fileExt = script.substr(pos);
			std::vector<BaseLocation *>::iterator it = _serverConfig.locationStruct.begin();
			for (; it != _serverConfig.locationStruct.end(); ++it) 
			{
				LocationFiles *file = dynamic_cast<LocationFiles *>(*it);

				// Check if the file is the same type as uri
				if (file && (file->name.find(fileExt) != std::string::npos) && file->cgi_pass.find(script) != std::string::npos) 
				{
					if (std::find(file->allow_methods.begin(), file->allow_methods.end(), _envp.request_method.substr(_envp.request_method.find_first_of('=') + 1)) != file->allow_methods.end()) 
					{
						executeCGIScript("./cgi-bin" + script, req, fd, resp);
						_isCGI = true;
						return 0;
					}
					return 405;
				}

				// If it's not on root level, check every subdirectory location level to see if the file is inside it
				else 
				{
					LocationDir *dir = dynamic_cast<LocationDir *>(*it);
					if (dir) 
					{
						std::vector<LocationFiles *>::iterator file_it = dir->files.begin();
						for (; file_it != dir->files.end(); ++file_it) 
						{
							LocationFiles *file = *file_it;

							// Check if the file is the same type as uri
							if (file && (file->name.find(fileExt) != std::string::npos) && file->cgi_pass.find(script) != std::string::npos) 
							{
								if (std::find(file->allow_methods.begin(), file->allow_methods.end(), _envp.request_method.substr(_envp.request_method.find_first_of('=') + 1)) != file->allow_methods.end()) 
								{
									executeCGIScript("./cgi-bin" + script, req, fd, resp);
									_isCGI = true;
									return 0;
								}
								return 405;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

/**
 * @brief Tests if the request requires CGI processing and executes the corresponding CGI script if found.
 *
 * This function tests if the request requires CGI processing (DELETE, POST, or GET) and executes the corresponding CGI script if found.
 * It first checks if the request method is DELETE and calls `deleteCGI`, then POST and calls `postCGI`, and finally GET and calls `getCGI`.
 * After execution, it sends the appropriate response back to the client.
 *
 * @param uri The URI from the request.
 * @param fd The file descriptor of the client connection socket.
 * @param req The request object.
 * @param resp The response object.
 * @param reqCode Reference to the request code.
 * @return An integer representing the status code.
 */
int Server::CGI(const std::string& uri, int fd, Request& req, Response& resp, int& reqCode) 
{
	int cgi = 0;

	deleteCGI(uri, fd, req, resp);
	if (_isCGI)
		return 0;
	cgi = postCGI(uri, fd, req, resp, reqCode);
	if (_isCGI)
		return 0;
	else if (cgi == 405)
		return 405;
	cgi = getCGI(uri, fd, req, resp);
	if (_isCGI)
		return 0;
	else if (cgi == 405 || cgi == 404)
		return cgi;
	_isCGI = false;
	return 0;
}

/**
 * @brief Executes a DELETE CGI script.
 *
 * This function is responsible for executing a DELETE CGI script. It forks a new process and in the child process, it prepares the necessary environment variables
 * and executes the CGI script. In the parent process, it waits for the child process to finish, and then sends the appropriate response back to the client.
 *
 * @param scriptPath The file path to the CGI script.
 * @param req The request object.
 * @param fd The file descriptor of the client connection socket.
 * @param resp The response object.
 */
void Server::executeDeleteCGIScript(const std::string& scriptPath, Request& req, int fd, Response &resp)
{
	int reqCode;
	std::string filename = req.trimValue("File-Name");
	if (filename.empty())
		reqCode = 204;
	std::string fullPath = "./Data/" + filename;
	struct stat buf;
	int exists = stat(fullPath.c_str(), &buf);
	if (exists != 0)
	{
		reqCode = 404;
		resp.sendResponse(this, fd, resp.getErrorPage(reqCode, getConf()), reqCode);
	}
	pid_t pid = fork();  // Create a new process
    if (pid == -1) 
	{
        std::cout << "Failed to fork." << std::endl;
    } else if (pid > 0) 
	{
        const int timeout = 5;
        time_t start = time(NULL);
        int status;
        while (1) {
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == -1)
                break ;
            else if (result == 0) {
                time_t currentTime = time(NULL);
                if (difftime(currentTime, start) >= timeout) {
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    break;
                }
            }
            usleep(100000);
        }
        if (WIFEXITED(status) ) 
		{
			resp.sendResponse(this, fd, "./var/www/html/form/delete.html", 202);
			return;
        }
    } 
	else 
	{
		std::string filename = req.trimValue("File-Name");
		std::string filenameEnv = "FILENAME=" + filename;
		char *argv[] = {const_cast<char*> ("php-cgi"), (char *)scriptPath.c_str(), NULL};
		char* envp[] = {const_cast<char*>(filenameEnv.c_str()), NULL}; 
		dup2(fd, STDOUT_FILENO);
		close(fd);
		if (execve("/usr/bin/php-cgi", argv, envp) == -1)
		{
			delete this;
			exit (404);
		}
		delete this;
		exit (202);
	}
}

/**
 * @brief Executes an upload CGI script.
 *
 * This function executes an upload CGI script. It creates two pipes for communication between the parent and child processes.
 * In the child process, it redirects stdin and stdout and executes the CGI script. In the parent process, it writes POST data to the child process and
 * waits for it to finish. Finally, it sends the appropriate response back to the client.
 *
 * @param scriptPath The file path to the CGI script.
 * @param req The request object.
 * @param fd The file descriptor of the client connection socket.
 * @param resp The response object.
 */
void Server::executeUploadCGIScript(const std::string& scriptPath, Request& req, int fd, Response &resp)
{

	//Here we create two pipes, to allow communication between parent-child and child-parent
	int toChild[2];
	int toParent[2];
	pipe(toChild);
	pipe(toParent);

	//Child process to execute the CGI Scripts
	pid_t pid = fork();
	if (pid == 0)
	{ // Child process
		close(toChild[1]); // Close unused write end of input pipe
		close(toParent[0]); // Close unused read end of output pipe

		// Redirect stdin from toChild and stdout to toParent
		dup2(toChild[0], STDIN_FILENO);
		dup2(toParent[1], STDOUT_FILENO);

		//Obtain the name of the file we're trying to upload
		std::string filename = req.getFilename();
		if (filename.empty())
			exit(EXIT_FAILURE);
		std::string filenameEnv = "FILENAME=" + filename;
		// Prepare environment variables if necessary
		char* envp[] = {const_cast<char*>(filenameEnv.c_str()), NULL};
		// Command to execute Python script
		char* argv[] = {const_cast<char*>(scriptPath.c_str()), const_cast<char*>(scriptPath.c_str()), NULL};

		execve("/usr/bin/python3", argv, envp);
		exit(EXIT_FAILURE);
    }
	else if (pid > 0)
	{ // Parent process
		close(toChild[0]); // Close the read end of the input pipe
		close(toParent[1]); // Close the write end of the output pipe

		// Write POST data to the CGI script (Assuming binary data)
		const std::string& postData = req.getBody(); // Check if getReqBody() actually retrieves raw binary data correctly
		write(toChild[1], postData.data(), postData.size());
		close(toChild[1]); // Close the write end to signal EOF to the child
		int reqCode;
		const int timeout = 5;
        time_t start = time(NULL);
        int status;
        while (1) {
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == -1)
                break ;
            else if (result == 0) {
                time_t currentTime = time(NULL);
                if (difftime(currentTime, start) >= timeout) {
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    break;
                }
            }
            usleep(100000);
        }

		if (req.getFilename().empty())
			// Empty Media
			reqCode = 204;
		else {
			std::string filepath = "./Data/" + req.getFilename();

			struct stat buffer;
			if (stat(filepath.c_str(), &buffer) == 0) 
			{
				// Accepted upload
				resp.sendResponse(this, fd, "./var/www/html/form/upload.html", 202);
				return ;
			}
			else
				// Failed Upload
				reqCode = 404;
		}

		// When uploading an empty form, the browser doesn't update the webpage even though we
		// still send the response for the status code
		resp.sendResponse(this, fd, resp.getErrorPage(reqCode, getConf()), reqCode);

	}
}

/**
 * @brief Executes a CGI script.
 *
 * This function executes a CGI script. It creates two pipes for communication between the parent and child processes.
 * In the child process, it redirects stdin and stdout and executes the CGI script. In the parent process, it writes POST data to the child process and
 * waits for it to finish. Finally, it sends the appropriate response back to the client.
 *
 * @param scriptPath The file path to the CGI script.
 * @param req The request object.
 * @param fd The file descriptor of the client connection socket.
 * @param resp The response object.
 */

void	Server::executeCGIScript(const std::string& scriptPath, Request& req, int fd, Response& resp) 
{
	if (scriptPath.find("upload.py") != std::string::npos) 
	{
		executeUploadCGIScript(scriptPath, req, fd, resp);
		return ;
	}

	if (scriptPath.find("chunker") != std::string::npos) 
	{
		resp.sendResponse(this, fd, resp.getErrorPage(411, getConf()), 411);
		return ;
	}

	//Here we create two pipes, to allow communication between parent-child and child-parent
	int toChild[2];
	int toParent[2];
	pipe(toChild);
	pipe(toParent);

	//Child process to execute the CGI Scripts
    pid_t pid = fork();
    if (pid == 0)
	{
		close(toChild[1]); // Close unused write end of input pipe
		close(toParent[0]); // Close unused read end of output pipe
		dup2(toChild[0], STDIN_FILENO);
		dup2(toParent[1], STDOUT_FILENO);
		char* envp[18];
		envp[0] = const_cast<char*>(_envp.auth_mode.c_str());
		envp[1] = const_cast<char*>(_envp.content_length.c_str());
		envp[2] = const_cast<char*>(_envp.content_type.c_str());
		envp[3] = const_cast<char*>(_envp.gateway_interface.c_str());
		envp[4] = const_cast<char*>(_envp.path_info.c_str());
		envp[5] = const_cast<char*>(_envp.path_translated.c_str());
		envp[6] = const_cast<char*>(_envp.query_string.c_str());
		envp[7] = const_cast<char*>(_envp.remote_addr.c_str());
		envp[8] = const_cast<char*>(_envp.remote_host.c_str());
		envp[9] = const_cast<char*>(_envp.remote_ident.c_str());
		envp[10] = const_cast<char*>(_envp.remote_user.c_str());
		envp[11] = const_cast<char*>(_envp.request_method.c_str());
		envp[12] = const_cast<char*>(_envp.script_name.c_str());
		envp[13] = const_cast<char*>(_envp.server_name.c_str());
		envp[14] = const_cast<char*>(_envp.server_port.c_str());
		envp[15] = const_cast<char*>(_envp.server_protocol.c_str());
		envp[16] = const_cast<char*>(_envp.server_software.c_str());
		envp[17] = NULL;
	// Command to execute Python script
	char* argv[] = {const_cast<char*>("/usr/bin/python3"), const_cast<char*>(scriptPath.c_str()), NULL};
	if (execve("/usr/bin/python3", argv, envp) == -1)
		exit(EXIT_FAILURE);
	}
	else if (pid > 0)
	{ // Parent process
		close(toChild[0]); // Close the read end of the input pipe
		close(toParent[1]); // Close the write end of the output pipe

		// Write POST data to the CGI script (Assuming binary data)
		const std::string& postData = req.getBody(); // Check if getReqBody() actually retrieves raw binary data correctly
		write(toChild[1], postData.data(), postData.size());
		close(toChild[1]); // Close the write end to signal EOF to the child


		const int timeout = 5;
        time_t start = time(NULL);
        int status;
        while (1) {
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == -1)
                break ;
            else if (result == 0) {
                time_t currentTime = time(NULL);
                if (difftime(currentTime, start) >= timeout) {
                    kill(pid, SIGKILL);
                    waitpid(pid, &status, 0);
                    break;
                }
            }
            usleep(100000);
        }
		resp.sendResponseCGI(toParent[0], toParent[1], fd);
	}
}

/* ===================== Non-CGI POST and DELETE Functions ===================== */

/**
 * @brief Appends new commentDB to the "commentDB.txt" file.
 * 
 * This function reads the request body containing new comments and appends them to the "commentDB.txt" file.
 * If the file doesn't exist, it creates one. Each new comment is separated by a delimiter.
 *
 * @param req The Request object containing the new comment in its request body.
 */
void	Server::executePost(Request& req)
{
	std::ifstream infile("commentDB.txt");
	if(!infile.good())
	{
		std::ofstream outfile("commentDB.txt");
        if (!outfile.is_open()) 
		{
            std::cout << "Error creating file!\n";
            return;
        }
		outfile.close();
	}
	std::ofstream file("commentDB.txt", std::ios_base::app);
    if (!file.is_open()) 
	{
        std::cout << "Error opening file!\n";
        return;
    }
    // Write new data to the file
	std::time_t timestamp = std::time(NULL);
    char buff[50];
    std::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));
	file << buff << ": ";
    file << req.getBody();
	file << "───────────────── // ─────────────────" << std::endl;
    file.close();
	std::cout << "Comment added successfully" << std::endl;
}

/**
 * @brief Deletes the "commentDB.txt" file.
 * 
 * This function attempts to delete the "commentDB.txt" file from the filesystem.
 * If the deletion is successful, it prints a success message; otherwise, it prints an error message.
 */
void	Server::executeDeleteFile()
{
	const char* filename = "commentDB.txt";
	if(remove(filename) == 0)
		std::cout << "File " << filename << " deleted successfully." << std::endl;
	else
		std::cout << "Error deleting file" << std::endl;
}

/* ===================== Server HTTP I/O Functions ===================== */

/**
 * @brief Processes and sends responses to client requests.
 * 
 * This function handles incoming client requests, processes them, and sends appropriate responses back to the client.
 * It performs various checks such as request parsing, CGI execution, file operations, and error handling.
 * The response is generated based on the requested URI, method, and server configuration settings.
 * 
 * @param socket The file descriptor of the client socket.
 * @return int Always returns 0 to indicate successful completion.
 */
int	Server::sender(int socket) 
{
	std::string locationRoot;
	std::string uri;
	std::string possibleIndex;
	int reqCode = 0;
	// Creating shortcuts for objects to avoid continuous memory accessing
	Connection cnt = getConnection(socket);
	int fd = cnt.getConnectionFD();
	usleep(10);
	Request& req = cnt.getConnectionRequest();
	Response& resp = cnt.getConnectionResponse();
	if (req._isRequestComplete)
		return (0);
	resp.defaultFlags();
	if (req.fillHeader(fd)) 
	{
		if (req._isRequestComplete) 
		{
			reqCode = 200;
			resp.sendResponse(this, fd, resp.getErrorPage(reqCode, getConf()), reqCode);
			//return (0);
		}
	}
	// Check request size
	reqCode = req.checkClientBodySize(this);
	if (reqCode != 413) 
	{
		// Parse the request URI, METHOD and HTTP VERSION. Returns appropriate response codes
		reqCode = req.ConfigureRequest(this);
		std::string host(req.getHost());
		std::size_t pos = host.find(':');
		if (pos != std::string::npos)
			host = host.substr(0, pos);
		if (host == "localhost" || host == "127.0.0.1") {}
		else if (std::find(_serverConfig.server_name.begin(), _serverConfig.server_name.end(), host) == _serverConfig.server_name.end()) 
		{
			resp.sendResponse(this, fd, resp.getErrorPage(400, getConf()), 400);
			return (0);
		}
		uri = req.getURI();
		if (reqCode != 200)
		{
			resp.sendResponse(this, fd, resp.getErrorPage(reqCode, getConf()), reqCode);
			return (0);
		}
		int cgi = CGI(uri, fd, req, resp, reqCode);
		if (_isCGI == true) 
		{
			_isCGI = false;
			return 0;
		}
		else if (cgi == 405 || cgi == 404)
		{
			resp.sendResponse(this, fd, resp.getErrorPage(cgi, _serverConfig), cgi);
			_isCGI = false;
			return (0);
		}
		// If we reached this point than we're not using CGI
		if(req.getMethod() == "POST" && reqCode == 200 && req.getURI() == "/form")
			executePost(req);
		else if(req.getMethod() == "DELETE" && reqCode == 200)
			executeDeleteFile();
		else if(req.getMethod() != "GET")
			goto end;
		// Find the subdirectory root if it exists, or receive '404 Page Not Found' / '400 Bad Request'
		locationRoot = resp.findRoot(this, uri);
		// Check if we're trying to access directly a file, if we are, remove the last part of the file in the URI
		if (locationRoot == "404" || locationRoot == "400") 
		{
			std::string uri_tmp;
			// Ignore last / of uri
			if (uri.find_last_of('/') != 0 && uri.find_last_of('/') == uri.length() - 1)
				uri.erase(uri.find_last_of('/'));
			size_t lastSlashIndex = uri.find_last_of('/');
			if (lastSlashIndex != std::string::npos && lastSlashIndex != 0 && uri.size() >= 2) 
			{
				uri_tmp = uri.substr(0, lastSlashIndex);
				possibleIndex = uri.substr(lastSlashIndex + 1);
				if (uri_tmp[uri_tmp.size() - 1] == '/')
                    uri_tmp.erase(uri_tmp.size() - 1);
			}
			// This is in case we're trying to access a file directly at root level
			else if (uri.find_first_of('/') == uri.find_last_of('/')) 
			{
				uri_tmp = "/";
				possibleIndex = uri.substr(1, std::string::npos);
			}
			// Find the appropriate location path for the file
			locationRoot = resp.findRoot(this, uri_tmp);
			if (locationRoot == "404" || locationRoot == "400")
				reqCode = atoi(locationRoot.c_str());
			else
				uri = uri_tmp;
		}
		// If we got an error while retrieving the path for our response we update the response code
		if (locationRoot == "404" || locationRoot == "400")
			reqCode = atoi(locationRoot.c_str());
		else if (reqCode == 0)
			reqCode = 200;
	}
	end:
	// Here we check if any previous function have returned an error
	if (reqCode != 200 && reqCode != 202) 
	{
		// Finding the correct error page file path and sending the appropriate response
		std::string errorPath = resp.getErrorPage(reqCode, this->_serverConfig);
		resp.sendResponse(this, fd, errorPath, reqCode);
		return 0;
	}
	// If the current response code is '200 OK', initialize the parameters that we will use to send a response. By default server's root settings
	size_t indexSize = _serverConfig.index.size();
	std::vector<std::string>	indexes = _serverConfig.index;
	std::string	rootPath = _serverConfig.server_root;
	// Static bool for savestate related to request generated via directory list
	static bool wasListed;
	// If previously we have found a subdirectory location
	if (!locationRoot.empty()) 
	{
		LocationDir* dir = resp.getDir(this, uri);
		// Check if the subdirectory has index defined. If it doesn't use the root settings
		// If the subdirectory is a redirect it won't have index, but we have a check for this further down the line
		if (!dir->index.empty() || !possibleIndex.empty() || wasListed) 
		{
			indexSize = dir->index.size();
			indexes = dir->index;
			rootPath += locationRoot;
			wasListed = false;
		}
	}
	// Select the appropriate path and index file, or get an error code for '404 Page Not Found' / '400 Bad Request'
	//	 	We have these errors possible here because it can pass all of the previous check but the index file be missing from the system or we can have a bad redirect
	LocationDir* dir = resp.getDir(this, uri);
	std::string path = resp.selectIndexFile(this, fd, indexes, indexSize, rootPath, uri, !dir || dir->autoindex, possibleIndex);
	// Init wasListed to false
	wasListed = false;
	if (path == "LIST") 
	{
		int l_files = resp.generateDirListing(this, fd, "./var/www/html/" + locationRoot);

		// If found files within the directory of the location add blank to index vector so we can pass a check related to requests via dir listing
		if (l_files != -1)
			for (int i = 0; i < l_files; i++)
				dir->index.push_back("");
		wasListed = true;
	}
	// Basic checks if indexFile is empty, we have a redirect, or path has an error
	else 
	{
			if ((_serverConfig.indexFile.empty() && !resp.getRedirectFlag()) || path == "404" || path == "400") 
			{
				// Check path if path is a Bad Request, if not default to Page Not Found
				if (path == "400" || path == "404")
					resp.sendResponse(this, fd, resp.getErrorPage(atoi(path.c_str()), getConf()), atoi(path.c_str()));
				else
					resp.sendResponse(this, fd, resp.getErrorPage(200, getConf()), 200);
			}
			else
				resp.sendResponse(this, fd, (path + _serverConfig.indexFile), reqCode);
	}
	// If indexes were added via dir listing, remove them so we can access the list again recursively in the browser
	if (wasListed)
		while (!dir->index.empty())
			dir->index.pop_back();
    return 0;
}

/**
 * @brief Closes a connection and removes it from the epoll event loop.
 * 
 * This function closes the specified file descriptor (fd) and removes it from the epoll event loop.
 * It iterates over the event buffer to find the corresponding event and removes it using epoll_ctl.
 * Additionally, it removes the connection from the server's list of open connections (_connectionsVector),
 * erases the file descriptor from the ServerMap to prevent repeated entries, and resets the
 * file descriptor's activity in the TimeMap.
 * 
 * @param fd The file descriptor of the connection to be closed.
 * @param epoll_fd The file descriptor of the epoll instance.
 * @param event_buffer A buffer containing epoll events.
 * @param ServerMap A map containing file descriptors and corresponding Server objects.
 * @param TimeMap A map containing file descriptors and their last activity timestamps.
 * @return int Always returns 0 to indicate successful completion.
 */
int		Server::closeConnections(int fd, int epoll_fd, struct epoll_event* event_buffer, std::map<int, Server*>& ServerMap, std::map<int, time_t>& TimeMap) {

	for (int i = 0; i < MAX_EVENT_BUFFER; i++) 
	{
		if (event_buffer[i].data.fd == fd) 
		{
			if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event_buffer[i]) < 0)
				throw ServerException("fd could not be closed.");
			break ;
		}
	}
	close(fd);
	std::vector<Connection>::iterator it;
	for (it = _connectionsVector.begin(); it != _connectionsVector.end();) 
	{
		if (it->getConnectionFD() == fd)
			it = _connectionsVector.erase(it);
		else
			++it;
	}
	ServerMap.erase(fd);
	TimeMap.erase(fd);
	return 0;
}

/* ===================== Body Parsing Functions ===================== */

/**
 * @brief Fills the body of the server configuration with words from a given istringstream.
 * 
 * This function reads words from the provided istringstream and appends them to the body of the server configuration.
 * It iterates through the stream until it reaches the end, pushing each word onto the body vector.
 * After filling the body, it performs a check for balanced curly brackets in the body.
 * 
 * @param iss The istringstream containing words to be added to the body.
 * @return int Always returns 0 to indicate successful completion.
 */
int	Server::bodyFiller(std::istringstream& iss) 
{
	std::string word;
	while (iss >> word)
	{
		getDynamicBody().push_back(word); // Accessing the most recently added server
		word.clear();
	}
	bracketChecker();
	return (0);
}

/**
 * @brief Checks if the body of the server configuration has balanced curly brackets.
 * 
 * This function checks if the body of the server configuration has balanced curly brackets.
 * It iterates through the body vector, maintaining a stack to keep track of opening and closing curly brackets.
 * After iterating through the body, it ensures that the stack is empty and returns a status code accordingly.
 * 
 * @return int Returns 0 if the body has balanced curly brackets, otherwise returns -1.
 */
int		Server::bracketChecker()
{
	std::vector<std::string>::iterator itb = _body.begin();
	std::vector<std::string>::iterator ite = _body.end();
	std::vector<std::string> stack;
	if (*itb != "server")
		return (-1);
	while (itb != ite) 
	{
	  	if (*itb == "{" || *itb == "}")
	    	stack.push_back(*itb);
	  	itb++;
	}
	std::vector<std::string>::iterator it;
	if (stack.size() % 2 != 0)
	  	return (-1);
	for (it = stack.begin(); it != stack.end() && stack.size() > 2; it++) 
	{
	  	if ((*it).length() != 1)
	  	  return (-1);
	  	if (*it == "}") {
	  	  if (it == stack.begin() && *it == "}")
	  	    return (-1);
	  	  stack.erase(it);
	  	  stack.erase(it - 1);
	  	  it = stack.begin();
	  	}
	}
	if ((stack[0] != "{" || stack[1] != "}") && (stack.size() == 2))
		return (-1);
	stack.pop_back();
	stack.pop_back();
	if (!stack.empty())
	  	return (-1);
	return (0);
}

/* ===================== Exceptions ===================== */

Server::ServerException::ServerException(const std::string& error) 
{
	std::ostringstream err;
	err << BOLD << RED << "Error: " << error << RESET;
	_errMessage = err.str();
}

Server::ServerException::~ServerException() throw() {}

const char* Server::ServerException::what() const throw () 
{
	return (_errMessage.c_str());
}

/* ===================== Overloaders ===================== */

std::ostream& operator<<(std::ostream& os, const Server& server) 
{
	os << "listen: " << server.getListen().host << ":" << server.getListen().port << std::endl;
	os << "server_name: " << server.getConf().server_name.back() << std::endl;
	os << "root: " << server.getConf().server_root << std::endl;
	std::vector<std::string>::const_iterator it;
	for (it = server.getConf().index.begin(); it != server.getConf().index.end(); it++)
		os << "index: " <<  *it << " ";
	os << std::endl;
	for (it = server.getConf().allow_methods.begin(); it != server.getConf().allow_methods.end(); it++)
		os << "allow_methods: " << *it << " ";
	os << std::endl;
	std::map<int, std::string>::const_iterator itm;
	os << "error_pages: " << std::endl;
	for (itm = server.getConf().errorPages.begin(); itm != server.getConf().errorPages.end(); itm++)
		os << "	  error: " << itm->first << "    " << itm->second << std::endl;
	os << "client_max_body_size: " << server.getConf().client_max_body_size << std::endl;
	return os;
}


/* ===================== Auxiliary Functions ===================== */

/**
 * @brief Fills the CGI environment variables for a POST request.
 * 
 * This function populates the CGI environment variables required for processing a POST request by a CGI script.
 * It takes the server configuration, request URI, request object, and a structure to hold the environment variables as input.
 * Based on the provided inputs, it sets various CGI environment variables such as CONTENT_LENGTH, CONTENT_TYPE, REQUEST_METHOD, etc.
 * The populated environment variables are stored in the provided t_cgi_env structure.
 * 
 * @param svConf The server configuration containing relevant information.
 * @param uri The URI associated with the request.
 * @param envp The structure to hold the CGI environment variables.
 * @param req The request object containing request details.
 */
void    envCGI(const t_server_config& svConf, const std::string& uri, t_cgi_env& envp, Request& req) 
{
    envp.content_length = "CONTENT_LENGTH=" + req.getContentLen();
    envp.content_type = "CONTENT_TYPE=" + req.getContentType();
    envp.gateway_interface = "GATEWAY_INTERFACE=CGI/1.1";
    envp.query_string = "QUERY_STRING=";
    envp.script_name = "SCRIPT_NAME=." + uri;
    envp.path_info = "PATH_INFO=";
    envp.path_translated = "PATH_TRANSLATED=";
    envp.remote_addr = "REMOTE_ADDR=" + req.trimValue("Remote-Addr");
    envp.remote_host = "REMOTE_HOST=";
    envp.remote_ident = "REMOTE_IDENT=";
    envp.remote_user = "REMOTE_USER=";
    envp.request_method = "REQUEST_METHOD=" + req.getMethod();
    envp.server_name = "SERVER_NAME=" + svConf.server_name.back();
    envp.server_protocol = "SERVER_PROTOCOL=" + req.getHTTPVersion();
    envp.server_software = "SERVER_SOFTWARE=";
}
