/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "ZipkinCppTutorial.h"

struct AppStartUp {
    AppStartUp() {
        srand((unsigned int)zipkin::utcmicrosecond());
    }
} __s_startup;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main() {
  boost::shared_ptr<THttpClient> socket(new THttpClient("10.16.28.37", 9411, "/api/v1/spans"));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  
  std::vector<zipkin::Span*> spans;
  auto root_span = zipkin::create("root");
  zipkin::Annotation an;
  an.host.service_name = "service-name";
  an.timestamp = zipkin::utcmicrosecond();
  an.value = "CppTutorial";
  root_span->annotations.push_back(an);
  spans.push_back(root_span);
  for (int i = 0; i < 10; i++) {
      auto span = zipkin::create(std::to_string(i), root_span);
      THRIFT_SLEEP_USEC(rand() % 100000);
      zipkin::finish(span);
      spans.push_back(span);
  }
  zipkin::finish(root_span);

  try {
    transport->open();

    /*
    t := thrift.NewTMemoryBuffer()
    p := thrift.NewTBinaryProtocolTransport(t)
    if err := p.WriteListBegin(thrift.STRUCT, len(spans)); err != nil {
        panic(err)
    }
    for _, s := range spans {
        if err := s.Write(p); err != nil {
            panic(err)
        }
    }
    if err := p.WriteListEnd(); err != nil {
        panic(err)
    }
    return t.Buffer
    */
    protocol->writeListBegin(T_STRUCT, spans.size());
    for (size_t i = 0; i < spans.size(); i++) {
        spans[i]->write(protocol.get());
    }
    protocol->writeListEnd();
    
    transport->flush();

    uint8_t buf[40960] = {};
    auto got = transport->read(buf, sizeof(buf));
    std::cout << "got=" << got << std::endl;
    transport->close();
  } catch (TException& tx) {
    std::cout << "ERROR: " << tx.what() << std::endl;
  }
#ifdef _WIN32
  system("pause");
#endif
}
