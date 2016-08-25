/*
 * idplugin.cc
 *
 *  Created on: Jan 17, 2014
 *      Author: fan
 */

#include <unistd.h>
#include <string>
#include <iostream>
#include <glog/logging.h>

#include <idgen/src/id.pb.h>
int main()
{
    int header;
    std::string body;
    while (read(0, &header, sizeof(header)) > 0) {
        body.resize(header);
        PCHECK(read(0, &body[0], header) == header);
        idgen::proto::Request req;
        if (!req.ParseFromString(body)) {
            body = "";
        } else {
            body = req.DebugString();
        }

        header = body.size();
        PCHECK(write(1, &header, sizeof(header)) == sizeof(header));
        PCHECK(write(1, &body[0], header) == header);

    }
    return 0;
}

