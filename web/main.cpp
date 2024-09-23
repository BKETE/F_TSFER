#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace fs = boost::filesystem;  // from <boost/filesystem.hpp>
using tcp = net::ip::tcp;          // from <boost/asio/ip/tcp.hpp>
using std::cout;
using std::endl;

// Directory to store uploaded files
const std::string upload_directory = "./uploads";

// Function to extract boundary from Content-Type
std::string extract_boundary(const std::string& content_type) {
    std::string boundary;
    auto pos = content_type.find("boundary=");
    if (pos != std::string::npos) {
        boundary = "--" + content_type.substr(pos + 9);
    }
    return boundary;
}

#include <boost/asio.hpp>
#include <iostream>

void print_socket_info(const tcp::socket& socket) {
    if (socket.is_open()) {
        std::cout << "Socket is open." << std::endl;

        auto local_endpoint = socket.local_endpoint();
        auto remote_endpoint = socket.remote_endpoint();

        std::cout << "Local endpoint: " << local_endpoint.address().to_string() << ":" << local_endpoint.port() << std::endl;
        std::cout << "Remote endpoint: " << remote_endpoint.address().to_string() << ":" << remote_endpoint.port() << std::endl;
    }
    else {
        std::cout << "Socket is closed." << std::endl;
    }
}


// Function to get MIME type based on file extension
std::string get_mime_type(const std::string& path) {
    std::string extension = path.substr(path.find_last_of('.') + 1);
    if (extension == "html") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    return "application/octet-stream";
}

// Function to send a file to the client
void send_file(tcp::socket& socket, const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file) {
        std::ostringstream oss;
        oss << file.rdbuf();  // Read the file content
        std::string file_content = oss.str();

        http::response<http::string_body> res{ http::status::ok, 11 };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, get_mime_type(path));
        res.body() = file_content;
        res.content_length(res.body().size());
        res.keep_alive(true);
        http::write(socket, res);
    }
    else {
        http::response<http::string_body> res{ http::status::not_found, 11 };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "text/plain");
        res.body() = "File not found";
        res.content_length(res.body().size());
        res.keep_alive(true);
        http::write(socket, res);
    }
}

// Function to parse multipart form data
bool parse_multipart_form_data(const std::string& body, const std::string& boundary, std::string& filename, std::string& file_content) {
    // ���ұ߽�
    auto boundary_start = body.find(boundary);
    if (boundary_start == std::string::npos) return false;

    // �ҵ��ļ���
    auto filename_start = body.find("filename=\"", boundary_start);
    if (filename_start == std::string::npos) return false;
    filename_start += 10; // ���� "filename=\""
    auto filename_end = body.find("\"", filename_start);
    if (filename_end == std::string::npos) return false;
    filename = body.substr(filename_start, filename_end - filename_start);



    // ���л����İ�ȫ��飬��ֹĿ¼����
    if (filename.find("..") != std::string::npos) {
        return false; // ��ֹʹ�����·��
    }

    // �ҵ����ݿ�ʼ��λ��
    auto content_start = body.find("\r\n\r\n", filename_end);
    if (content_start == std::string::npos) return false;
    content_start += 4; // ����ͷ��

    // �ҵ���һ���߽�
    auto boundary_end = body.find(boundary, content_start);
    if (boundary_end == std::string::npos) return false;

    // ȷ���ļ����ݽ�����λ��
    auto file_content_length = boundary_end - content_start - 2; // ȥ����β�� \r\n
    if (file_content_length < 0) return false;

    // ��ȡ�ļ�����
    file_content = body.substr(content_start, file_content_length);
    //cout <<  file_content << endl;
    return true;
}


// Function to handle file upload
void handle_upload(tcp::socket& socket, const std::string& body, const std::string& content_type) {
    cout << "ok" << endl;
    std::string boundary = extract_boundary(content_type);
    if (boundary.empty()) {
        http::response<http::string_body> res{ http::status::bad_request, 11 };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Invalid boundary";
        res.content_length(res.body().size());
        res.keep_alive(true);
        http::write(socket, res);
        return;
    }

    std::string filename;
    std::string file_content;

    bool bl = parse_multipart_form_data(body, boundary, filename, file_content);
    cout << bl << endl;

    if (parse_multipart_form_data(body, boundary, filename, file_content)) {
        try {
            std::ofstream file(upload_directory + "/" + filename, std::ios::out | std::ios::binary);
            if (file) {
                //cout << file_content << endl;
                file << file_content; // Write file content
                http::response<http::string_body> res{ http::status::ok, 11 };
                res.set(http::field::server, "Beast");
                res.set(http::field::content_type, "text/plain");
                res.body() = "File uploaded successfully";
                res.content_length(res.body().size());
                res.keep_alive(true);
                http::write(socket, res);
            }
            else {
                http::response<http::string_body> res{ http::status::internal_server_error, 11 };
                res.set(http::field::server, "Beast");
                res.set(http::field::content_type, "text/plain");
                res.body() = "Failed to save the file";
                res.content_length(res.body().size());
                res.keep_alive(true);
                http::write(socket, res);
            }
        }
        catch (const std::exception& e) {
            http::response<http::string_body> res{ http::status::internal_server_error, 11 };
            res.set(http::field::server, "Beast");
            res.set(http::field::content_type, "text/plain");
            res.body() = "Error: " + std::string(e.what());
            res.content_length(res.body().size());
            res.keep_alive(true);
            http::write(socket, res);
        }
    }
    else {
        http::response<http::string_body> res{ http::status::bad_request, 11 };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Failed to parse multipart data";
        res.content_length(res.body().size());
        res.keep_alive(true);
        http::write(socket, res);
    }
}

// Function to handle incoming requests
void handle_request(tcp::socket& socket) {

    // ����������
    http::request_parser<http::string_body> parser;

    // �����������С���ƣ����� 50MB
    parser.body_limit(50 * 1024 * 1024);  // 50MB
    cout << "handle_request" << endl;
    print_socket_info(socket);
    beast::flat_buffer buffer; // Set buffer size limit to 10 MB
    http::request<http::string_body> req;

    try {
        http::read(socket, buffer, parser);
        req = parser.get();
    }
    catch (const beast::system_error& se) {
        cout << 123 << endl;
        std::cerr << "Error: " << se.what() << "\n";
        // ���ʹ�����Ӧ���ͻ���
        http::response<http::string_body> res{ http::status::bad_request, 11 };
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Request error: " + std::string(se.what());
        res.content_length(res.body().size());
        res.keep_alive(true);
        http::write(socket, res);
        return;
    }


    std::string path = req.target();
    if (path == "/") path = "/upload.html";  // Default path

    if (req.method() == http::verb::post && path == "/upload") {
        auto content_type = req[http::field::content_type];
        if (content_type.find("multipart/form-data") != std::string::npos) {
            handle_upload(socket, req.body(), content_type);
        }
        else {
            http::response<http::string_body> res{ http::status::unsupported_media_type, 11 };
            res.set(http::field::server, "Beast");
            res.set(http::field::content_type, "text/plain");
            res.body() = "Unsupported Content-Type";
            res.content_length(res.body().size());
            res.keep_alive(true);
            http::write(socket, res);
        }
    }
    else {
        // Serve static files or handle other requests
        send_file(socket, "." + path);
    }
}

int main() {
    try {
        std::setlocale(LC_ALL, "en_US.UTF-8");  // ���ó���ʹ�� UTF-8 ����
        net::io_context io_context;
        tcp::acceptor acceptor{ io_context, {tcp::v4(), 12005} };

        // Ensure upload directory exists
        if (!fs::exists(upload_directory)) {
            fs::create_directory(upload_directory);
        }

        for (;;) {
            tcp::socket socket{ io_context };
            acceptor.accept(socket);
            handle_request(socket);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}