#include "webserv.hpp"
extern char **environ;

int CGI::execute(Client *cli)
{
	if (cli->already_visite_cgi == false)
	{
		cli->cig_start_time = time(NULL);
		cli->already_visite_cgi = true;
		init();

		if ((_cgi_child = fork()) == -1) // fork process
			throw 500;
		if (_cgi_child == 0) // child process
		{
			if ((_in_file_fd = open(_body_path.c_str(), O_RDONLY | O_CREAT, 0666)) == -1){ // open input file
				exit(EXIT_FAILURE);
			}
			if ((_err_file_fd = open(ERROR_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1){ // open error file
				close(_in_file_fd);
				exit(EXIT_FAILURE);
			}
			if ((_out_file_fd = open(_ouput_file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666)) == -1){ // open output file
				close(_in_file_fd);
				close(_err_file_fd);
				exit(EXIT_FAILURE);
			}
			if (-1 == dup2(_in_file_fd, STDIN_FILENO) ){
				close(_in_file_fd);
				close(_err_file_fd);
				exit(EXIT_FAILURE);
			}
			if (dup2(_out_file_fd, STDOUT_FILENO) == -1 || -1 == dup2(_err_file_fd, STDERR_FILENO)){ // redirect stdout and stderr to pipe
				close(_in_file_fd);
				close(_err_file_fd);
				exit(EXIT_FAILURE);
			}
			if (close(_out_file_fd) || close(_err_file_fd) == EOF || EOF == close(_in_file_fd)){ // close pipe
				exit(EXIT_FAILURE);
			}
			const char *argv[] = {_cgi_path.c_str(), _path.c_str(), NULL};
			execve(_cgi_path.c_str(), (char **)argv, environ);
			exit(EXIT_FAILURE);
		}
		close(_err_file_fd);
	}
	{
		int r_status = waitpid(_cgi_child, &_status, WNOHANG);
		if (r_status == -1)
			throw 500;
		if (r_status == _cgi_child){
			return (0);
		}
		std::time_t elapsed = std::time(0) - cli->cig_start_time;
		if (elapsed >= TIMEOUT){
			if (kill(_cgi_child, SIGKILL) == -1) // Kill child process
				throw 500;
			waitpid(_cgi_child, NULL, WNOHANG);
			throw 408;
		}
		return (STILL_RUNNING);
	}
	return (0);
}

void	CGI::init(void)
{
	string file_path = get_CGI_script(_req.get_file_name(), _req.get_server_index(), 0);
	_err_file_fd = open(ERROR_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666); // open error file
	if (_err_file_fd == -1)
		throw 500;
	_body_path = _req.get_body_path();
	_path = _loc.getRoot() + get_CGI_script(file_path, _req.get_server_index(), 0);
	if (access(_path.c_str(), F_OK) == -1)
		throw 404;
	_filename = file_path;
	_cgi_path = _loc.getCgi().at(_filename.substr(_filename.find_last_of('.')));
	_content_length = _req.get_fixed_length();
	string _query_string = _req.get_URI().substr((_req.get_URI().find('?' ) == string::npos ? _req.get_URI().size() : _req.get_URI().find('?')));
	if (!_query_string.empty())
		_query_string = _query_string.substr(1);
	_ouput_file_path = generate_random_name();
	(
		setenv("REDIRECT_STATUS", "", 1) |
		setenv("SERVER_SOFTWARE", "Webserv 42 (1337)", 1) |
		setenv("HTTP_COOKIE", _req.get_header("Cookie").c_str(), 1) |
		setenv("SERVER_NAME", _req.get_Host().c_str(), 1) |
		setenv("GATEWAY_INTERFACE", "CGI/1.1", 1) |
		setenv("SERVER_PROTOCOL", "HTTP/1.1", 1) |
		setenv("SERVER_PORT", DEL::to_string(servers.at(_req.get_server_index()).getPort()).c_str(), 1) |
		setenv("REQUEST_METHOD", _req.get_method().c_str(), 1) |
		setenv("SCRIPT_FILENAME", _path.c_str(), 1) |
		setenv("PATH_TRANSLATED", _path.c_str(), 1) |
		setenv("PATH_INFO", _req.get_file_name().substr(file_path.size()).c_str(), 1) |
		setenv("SCRIPT_NAME", _path.c_str(), 1) |
		setenv("CONTENT_TYPE", _req.get_header("Content-Type").c_str(), 1) |
		setenv("CONTENT_LENGTH", DEL::to_string(_content_length).c_str(), 1) |
		setenv("QUERY_STRING", _query_string.c_str(), 1)
	) == -1 ? throw 500 : 0;
}

string		CGI::get_response(void)
{
	ifstream file_in(_ouput_file_path.c_str());
	if (!file_in.is_open())
		throw 500;
	string output((istreambuf_iterator<char>(file_in)), istreambuf_iterator<char>());
	file_in.close();
	unlink(_ouput_file_path.c_str());
	_output = output;
	string	output_file_path = generate_random_name();
	unsigned long long	content_length = SIZE_MAX;
	string		status_code;
	string		response_headers;
	string	content;
	int boundary_len = (_output.find("\n\n") > _output.find("\r\n\r\n") ? 4: 2);
	size_t	heaers_end_pos =  (_output.find("\n\n") > _output.find("\r\n\r\n") ? _output.find("\r\n\r\n"): _output.find("\n\n"));
	ofstream file(output_file_path.c_str());
	if (heaers_end_pos == string::npos){
		content = _output;
	}
	else
	{
		string headers = _output.substr(0, heaers_end_pos);
		vector<string> header_lines = split_string_with_multiple_delemetres(headers, "\r\n");
		for (size_t i = 0; i < header_lines.size(); i++)
		{
			vector<string> header = split_string_with_multiple_delemetres(header_lines[i], ":");
			if (header.size() == 2)
			{
				if (header.at(0) == "Content-Length"){
					size_t j = 0;
					for (; j < header.at(1).size() && std::isspace(header.at(1)[j]); j++); // skip whiltespaces
					for (; j < header.at(1).size(); j++)
						if (isdigit(header.at(1)[j]) == 0)
							throw 501;
					if (header.at(1).size() > 18)
						throw 500;
					content_length = atoll(header.at(1).c_str());
				}
				else if (header.at(0) == "Status" && status_code.empty())
					status_code = header.at(1);
				else
					response_headers +=  header_lines[i] += "\r\n";
			}
			else
				response_headers +=  header_lines[i] += "\r\n";

		}
		if (content_length == SIZE_MAX || content_length  > _output.length() - heaers_end_pos)
			content = _output.substr(heaers_end_pos + boundary_len);
		else
			content = _output.substr(heaers_end_pos + boundary_len, content_length);
	}
	file << "HTTP/1.1 " << (status_code.empty() ? "200 OK" : status_code) << "\r\n";
	file << "Server: webserv/29.16666666729\r\n";
	file << response_headers;
	file << "Content-Length: " << content.length() << "\r\n";
	file << "\r\n";
	file << content;
	file.close();
	return (output_file_path);
}