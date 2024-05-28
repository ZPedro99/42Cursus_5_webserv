 /* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cluster.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lde-sous <lde-sous@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/02/05 14:36:38 by lde-sous          #+#    #+#             */
/*   Updated: 2024/02/05 14:36:38 by lde-sous         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLUSTER_HPP
# define CLUSTER_HPP

# pragma once
# include "../webserv.hpp"
# include "../structures.hpp"
# include "../config/Configuration.hpp"
# include "Server.hpp"
# include "../requests/Request.hpp"

class Server;
class Configuration;

class	Cluster
{
	private:
		Cluster();
		size_t					_numberOfSv;
		std::vector<Server*>	 _servers;
		std::vector<pollfd> 	_pollFDs;
		std::map<int, Server*> 	_fdMap;
		std::map<int, time_t> 	_activityTime;
		std::vector<int>		_serverSockets;
		Configuration			_config;

	public:
		explicit Cluster(const std::string& filepath);
		Cluster(const Cluster& original);
		Cluster& operator=(const Cluster& original);
		~Cluster();
		void	setPollFD();
		Server* getServer();
		Server* getServer(int fd);
		std::vector<Server*>& getServers();
		void	initServer();
		void	configureServer(Server* server);
		void	createServer(t_listen& listenStruct);
		void	setupServer();
		void	promptInfo();
		void	clearServer();
		int	 	checkTimeout(int epoll_fd, struct epoll_event* event_buffer);
		void	processSemicolon(Server* server);
		void	processDoubles(Server *server);
		void	processServerName(Server* server);
		void	processListen(std::string& wordStack, t_listen& listen);
		void	processServerRoot(Server* server);
		void	processErrorPage(Server* server);
		void	processIndex(Server* server);
		void	processMethods(Server* server);
		void	processClientSize(Server* server);
		void	processLocations(Server* server);
		void	startServers();
		void	connectionHandler(int fd, Server* server);

		class ClusterException : public std::exception
		{
			private:
				std::string _errMessage;
			public:
				ClusterException(const std::string& error);
				~ClusterException() throw();
				virtual const char* what() const throw();
		};
};

#endif
