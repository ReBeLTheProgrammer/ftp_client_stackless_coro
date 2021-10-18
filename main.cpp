#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/yield.hpp>
#include <boost/program_options.hpp>

namespace ba = boost::asio;

class ClientControlConnection {
public:
    ClientControlConnection(const ba::ip::tcp::endpoint& ep, const ba::thread_pool::executor_type& context):
        _ep(ep),
        _context(context),
        _currentSock(std::make_shared<ba::ip::tcp::socket>(context)),
        _dataSock(std::make_shared<ba::ip::tcp::socket>(context)),
        _file(std::make_shared<ba::posix::stream_descriptor>(context)),
              _buffer(std::make_shared<std::string>()),
        t1(std::make_shared<ba::chrono::steady_clock::time_point>()),
        t2(std::make_shared<ba::chrono::steady_clock::time_point>()){ }

    void operator() (const boost::system::error_code& ec = boost::system::error_code(), std::size_t bytes_transferred = 0){
        reenter(_coro){
            yield _currentSock->async_connect(_ep, *this);
            if(!_currentSock->is_open()) {
                yield break;
            }

            //Attempt to read a welcome message
            do {
                _buffer->erase(0, _buffer->find("\r\n") + 2);
                _buffer->reserve(500);
                if(_buffer->find("\r\n") == std::string::npos){
                    yield {
                        ba::dynamic_string_buffer buf(*_buffer, 500);
                        ba::async_read_until(*_currentSock, buf, '\n', *this);
                    }
                    if (ec) {
                        yield break;
                    }
                }
                //std::cout << *_buffer;
            } while(_buffer->find(' ') != 3);
            if(_buffer->substr(0, 3) != "220") {
                yield break;
            }

            //Attempt to send a login message
            *_buffer = "USER anonymous\r\n";
            yield ba::async_write(*_currentSock, ba::buffer(*_buffer, _buffer->size()), *this);
            if(ec){
                yield break;
            }

            //Attempt to parse a login answer
            do {
                _buffer->erase(0, _buffer->find("\r\n") + 2);
                _buffer->reserve(500);
                if(_buffer->find("\r\n") == std::string::npos){
                    yield {
                                                            ba::dynamic_string_buffer buf(*_buffer, 500);
                                                            ba::async_read_until(*_currentSock, buf, '\n', *this);
                                                        }
                    if (ec) {
                        yield break;
                    }
                }
                //std::cout << *_buffer;
            } while(_buffer->find(' ') != 3);
            if(_buffer->substr(0, 3) != "230") {
                yield break;
            }

            //Attempt to change representation type
            *_buffer = "TYPE I\r\n";
            yield ba::async_write(*_currentSock, ba::buffer(*_buffer, _buffer->size()), *this);
            if(ec){
                yield break;
            }

            //Attempt to TYPE result
            do {
                _buffer->erase(0, _buffer->find("\r\n") + 2);
                _buffer->reserve(500);
                if(_buffer->find("\r\n") == std::string::npos){
                    yield {
                                                            ba::dynamic_string_buffer buf(*_buffer, 500);
                                                            ba::async_read_until(*_currentSock, buf, '\n', *this);
                                                        }
                    if (ec) {
                        yield break;
                    }
                }
                //std::cout << *_buffer;
            } while(_buffer->find(' ') != 3);
            if(_buffer->substr(0, 3) != "200") {
                yield break;
            }

            //Attempt to send PASV command
            *_buffer = "PASV\r\n";
            yield ba::async_write(*_currentSock, ba::buffer(*_buffer, _buffer->size()), *this);
            if(ec){
                yield break;
            }

            //Attempt to parse a PASV answer
            do {
                _buffer->erase(0, _buffer->find("\r\n") + 2);
                _buffer->reserve(500);
                if(_buffer->find("\r\n") == std::string::npos){
                    yield {
                                                            ba::dynamic_string_buffer buf(*_buffer, 500);
                                                            ba::async_read_until(*_currentSock, buf, '\n', *this);
                                                        }
                    if (ec) {
                        yield break;
                    }
                }
                //std::cout << *_buffer;
            } while(_buffer->find(' ') != 3);
            if(_buffer->substr(0, 3) != "227") {
                yield break;
            }
            yield{
                std::string sub = _buffer->substr(_buffer->find('('), _buffer->find(')') - _buffer->find('(') + 1);
                int pos = 0;
                for(int i = 0; i < 4; i++)
                    pos = sub.find(',', pos) + 1;
                sub = sub.substr(pos, sub.find(')') - pos);
                sub[sub.find(',')] = ' ';
                std::istringstream ss(sub);
                std::uint16_t hi, lo;
                ss >> hi >> lo;
                _dataSock->async_connect(ba::ip::tcp::endpoint(_ep.address(), (hi << 8) | lo), *this);
            }
            if(ec){
                yield break;
            }

            //Attempt to get file
            *_buffer = "RETR 1\r\n";
            yield ba::async_write(*_currentSock, ba::buffer(*_buffer, _buffer->size()), *this);
            if(ec){
                yield break;
            }

            //Attempt to parse connection start
            do {
                _buffer->erase(0, _buffer->find("\r\n") + 2);
                _buffer->reserve(500);
                if(_buffer->find("\r\n") == std::string::npos){
                    yield {
                        ba::dynamic_string_buffer buf(*_buffer, 500);
                        ba::async_read_until(*_currentSock, buf, '\n', *this);
                    }
                    if (ec) {
                        yield break;
                    }
                }
                //std::cout << *_buffer;
            } while(_buffer->find(' ') != 3);
            if(_buffer->substr(0, 3) != "150") {
                yield break;
            }

//            _file->assign(open("1", O_WRONLY | O_CREAT | 0660));

            *t1 = ba::chrono::steady_clock::now();
            //Get file data
            do {
                _buffer->resize(65500);
                yield _dataSock->async_receive(ba::buffer(*_buffer, _buffer->size()), *this);
//                if(bytes_transferred != 0){
//                    _buffer->resize(bytes_transferred);
//                    yield ba::async_write(*_file, ba::buffer(*_buffer, _buffer->size()), *this);
//                }
            } while(bytes_transferred != 0 && !ec);
            *t2 = ba::chrono::steady_clock::now();
            _buffer->erase();

//            //Attempt to parse connection end
//            do {
//                _buffer->erase(0, _buffer->find("\r\n") + 2);
//                _buffer->reserve(500);
//                if(_buffer->find("\r\n") == std::string::npos){
//                    yield {
//                        ba::dynamic_string_buffer buf(*_buffer, 500);
//                        ba::async_read_until(*_currentSock, buf, '\n', *this);
//                    }
//                    if (ec) {
//                        yield break;
//                    }
//                }
                _dataSock->close();
//                //std::cout << *_buffer;
//            } while(_buffer->find(' ') != 3);
//            if(_buffer->substr(0, 3) != "250") {
//                yield break;
//            }
//
            //Attempt to end session
            *_buffer = "QUIT\r\n";
            yield ba::async_write(*_currentSock, ba::buffer(*_buffer, _buffer->size()), *this);
            if(ec){
                yield break;
            }
//
//            //Attempt to parse connection end
//            do {
//                _buffer->erase(0, _buffer->find("\r\n") + 2);
//                _buffer->reserve(500);
//                if(_buffer->find("\r\n") == std::string::npos){
//                    yield {
//                        ba::dynamic_string_buffer buf(*_buffer, 500);
//                        ba::async_read_until(*_currentSock, buf, '\n', *this);
//                    }
//                    if (ec) {
//                        yield break;
//                    }
//                }
//                //std::cout << *_buffer;
//            } while(_buffer->find(' ') != 3);
//            if(_buffer->substr(0, 3) != "221") {
//                yield break;
//            }
            do{
                _buffer->resize(100, 0);
                yield _currentSock->async_receive(ba::buffer(*_buffer, _buffer->size()), *this);
            } while(bytes_transferred > 0);
            std::cout << "Connection # " << ++closedCount << " has finished working\n";
            std::cout << "Time for data transfer: " << ba::chrono::duration_cast<ba::chrono::milliseconds>(*t2 - *t1).count() << " milliseconds\n";
            _currentSock->close();
        }
    }

    std::shared_ptr<ba::chrono::steady_clock::time_point> t1, t2;

private:
    ba::ip::tcp::endpoint _ep;
    ba::thread_pool::executor_type _context;
    std::shared_ptr<ba::ip::tcp::socket> _currentSock, _dataSock;
    std::shared_ptr<std::string> _buffer;
    ba::coroutine _coro;
    std::shared_ptr<ba::posix::stream_descriptor> _file;
    inline static int closedCount = 0;
};

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    std::uint16_t tasksCount;
    std::uint16_t threadCount;
    std::uint16_t port;

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "print this help message")
            ("threads", po::value<std::uint16_t>(&threadCount)->default_value(std::thread::hardware_concurrency()), "set the maximum cores to be used")
            ("port",    po::value<std::uint16_t>(&port), "set the port for the control connections")
            ("tasks", po::value<std::uint16_t>(&tasksCount), "number of ftp connections to be posted");

    po::variables_map options;

    po::store(boost::program_options::parse_command_line(argc, argv, desc), options);
    po::notify(options);

    if(options.count("help")){
        std::cout << desc << '\n';
        return 1;
    }

    ba::thread_pool pool(3);
    std::vector<ClientControlConnection> tasks;
    ba::ip::tcp::endpoint ep(ba::ip::address::from_string("192.168.178.36"), port);
    for(std::size_t i = 0; i < tasksCount; i++) {
        tasks.emplace_back(ClientControlConnection(ep, pool.executor()));
    }
    for(std::size_t i = 0; i < tasksCount; i++)
        tasks[i]();
    pool.join();
    float timeMean = 0;
    for(std::size_t i = 0; i < tasksCount; i++){
        timeMean += float(ba::chrono::duration_cast<ba::chrono::milliseconds>(*(tasks[i].t2) - *(tasks[i].t1)).count()) / tasksCount;
    }
    std::cout << "Mean time for each connection is: " << timeMean << " milliseconds\n";
    return 0;
}
