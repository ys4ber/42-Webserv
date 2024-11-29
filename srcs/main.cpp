#include "webserv.hpp"

vector <Serv> servers;

int main(int ac, char **av)
{
    try
    {
        parse_config(ac, av);
        _Create_Servers();
        _Run_Server();
    }
    catch (std::exception &e)
    {
        std::cerr << "\033[1;36m[WebServ]\033[0m " << e.what() << std::endl;
    }
    return 0;
}

