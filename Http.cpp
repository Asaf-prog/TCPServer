#include "HTTP.h"

void FindWantedFile(string path, struct Request* req, string* cmd) 
{ 
	// detecting query strings and extracting file name
	
	int pathSize = path.size();

	for (int i = 1; i < pathSize; i++) 
	{
		if (path[i] != '?')
		{
			req->file.name.push_back(path[i]);
		}
		else 
		{
			if (path[i + 1] == 'l' && path[i + 2] == 'a' && path[i + 3] == 'n' && path[i + 4] == 'g' && path[i + 5] == '=') 
			{
				if (path[i + 6] == 'h' && path[i + 7] == 'e') 
				{
					req->file.language = "Hebrew";
				}
				else if (path[i + 6] == 'f' && path[i + 7] == 'r') 
				{
					req->file.language = "French";
				}
				else if (path[i + 6] == 'e' && path[i + 7] == 'n') 
				{
					req->file.language = "English";
				}
				else
				{
					req->file.language = "Not specified";
				}
			}
			else 
			{
				(*cmd) = "ERR";
			}
			break;
		}
	}
}

void ParseGetMsg(stringstream& mesStream, struct Request* req)
{
	string str = mesStream.str();
	struct Attribute currAtt = findNextAtt(mesStream, str);

	while (currAtt.key != "END") 
	{
		if (currAtt.key == "Accept-language") 
		{
			//just in case query strings are not available
			
			req->file.language = currAtt.data;
		}

		currAtt = findNextAtt(mesStream, str);
	}
}

struct Attribute findNextAtt(stringstream& msg, string input) 
{
	//returning the next attribute in http message
	
	struct Attribute att;
	int currIndex;
	
	msg >> att.key;

	if (att.key != "") 
	{
		att.key.pop_back();//remove ':' from key
		//Because value could be seperated by spaces, we inserting chars manually
		currIndex = msg.tellg();
		currIndex++;//avoiding the space after the ':'
		
		while (input[currIndex] != '\r') 
		{
			att.data.push_back(input[currIndex]);
			currIndex++;
		}

		currIndex += 2;//avoiding \n as well
		msg.seekg(currIndex);
	}
	else
	{
		att.key = "END";
	}

	return att;
}

void ParseMsg(stringstream& mesStream, struct Request* req) 
{
	// going through the entire request for further calculation
	
	string str = mesStream.str();
	struct Attribute currAtt = findNextAtt(mesStream, str);

	while (currAtt.key != "END") 
	{
		currAtt = findNextAtt(mesStream, str);
	}
}

void ParsePostMsg(stringstream& mesStream, struct Request* req) 
{
	string str = mesStream.str();
	struct Attribute currAtt = findNextAtt(mesStream, str);
	string temp;
	int len;

	while (currAtt.key != "END") 
	{
		if (currAtt.key == "Content-Length") 
		{  
			//if we have arrived to the body of the request
			
			req->postContent.length = currAtt.data;
			len = atoi(req->postContent.length.c_str());
			do 
			{
				mesStream >> temp;
				req->postContent.data.append(temp);
				req->postContent.data.push_back(' ');
			} while (req->postContent.data.size() < len);
			
			req->postContent.data.pop_back();//remove last space

			cout << "POST:\n" << req->postContent.data << endl;
			break;
		}
		else if (currAtt.key == "Content-Type") 
		{
			req->postContent.type = currAtt.data;
		}

		currAtt = findNextAtt(mesStream, str);
	}
}

void ParsePutMsg(stringstream& mesStream, struct Request* req) 
{
	string str = mesStream.str();
	struct Attribute currAtt = findNextAtt(mesStream, str);
	int len;
	string temp;

	while (currAtt.key != "END") 
	{
		if (currAtt.key == "Content-Length") 
		{ 
			//if we have arrived to the body of the request
			
			req->postContent.length = currAtt.data;
			len = atoi(req->postContent.length.c_str());
			do 
			{
				mesStream >> temp;
				req->file.content.append(temp);
				req->file.content.push_back(' ');
			} while (req->file.content.size() < len);
			req->file.content.pop_back();//remove last space
			break;
		}
		
		currAtt = findNextAtt(mesStream, str);
	}
}

string CreateMessage(struct SocketState socket) 
{
	stringstream message;
	time_t curr;
	string str;

	time(&curr);

	//start of the response is the same for every method
	message << "HTTP/1.1 " << socket.req.status << " ";

	switch (socket.req.status) 
	{
		case 200:
			message << "OK\n";
			break;
		case 201:
			message << "Created\n";
			break;
		case 204:
			message << "No Content\n";
			break;
		case 404:
			message << "Not Found\n";
			break;
		default:
			message << "status error\n";
	}

	message << "Content-Type: text/html\n" << "Connection: keep-alive\n" << "Date: " << ctime(&curr) << "Server: HTTP Web Server\n";

	switch (socket.req.method) 
	{
		case GET:
			if (socket.req.status == 200) 
			{
				message << "Content-Length: " << socket.req.file.content.size() << "\n" << "\n";
				message << socket.req.file.content;
			}
			else 
			{
				//404
				str = "404 NOT FOUND";
				message << "Content-Length: " << str.size() << "\n" << "\n";
				message << str;
			}
			break;
		case OPTIONS:
			message << "Allow: GET, POST, PUT, OPTIONS, DELETE, TRACE, HEAD\n";
			message << "Accept-Language: he, en, fr\n" << "\n";
			break;
		case HEAD:
			if (socket.req.status == 200) 
			{
				message << "Content-Length: " << socket.req.file.content.size() << "\n" << "\n";
			}
			else 
			{
				//404
				str = "404 NOT FOUND";
				message << "Content-Length: " << str.size() << "\n" << "\n";
			}
			break;
		case POST:
			str = "Sent POST response";
			message << "Content-Length: " << str.size() << "\n" << "\n";
			message << socket.req.postContent.data;
			break;
		case DEL:
			if (socket.req.status == 200)
				str = "File Deleted";
			else //404
				str = "404 NOT FOUND";
			message << "Content-Length: " << str.size() << "\n" << "\n";
			message << str;
			break;
		case TRACE:
			message << "Content-Length: " << strlen(socket.buffer) << "\n" << "\n";
			message << socket.buffer;
			break;
		case PUT:
			if (socket.req.status == 201)
				str = "File Created and Updated";
			else //200
				str = "File Overrun";
			message << "Content-Length: " << str.size() << "\n" << "\n";
			message << str;
			break;
		default://ERR
			str = "Method not supported by this surver. Available methods specified with 'OPTIONS'";
			message << "Content-Length: " << str.size() << "\n" << "\n";
			message << str;
			break;
	}

	return message.str();
}
