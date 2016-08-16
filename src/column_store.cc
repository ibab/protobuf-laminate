#include <H5Cpp.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>
#include <iostream>
#include <stack>
#include <tuple>
#include <vector>

#include "column_store.h"
#include "hdf_stream.h"

namespace laminate {

using google::protobuf::Message;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;

ColumnWriter::ColumnWriter(
    std::vector<const google::protobuf::FieldDescriptor*> fds,
    std::vector<std::string> names)
  : fds_(fds), names_(names) {}

// Performs a depth-first traversal of the protobuf definition,
// storing the field descriptors and fully qualified names of
// all leaf (== primitive) entries.
std::tuple<ColumnWriter, Status> ColumnWriter::Create(Message& m) {
  typedef std::tuple<const FieldDescriptor*, std::string> Info;
  const Descriptor* desc = m.GetDescriptor();
  std::stack<Info> stack;
  std::vector<const FieldDescriptor*> fds;
  std::vector<std::string> names;
  for (int i = 0; i < desc->field_count(); i++) {
    stack.push(Info(desc->field(i), desc->field(i)->name()));
  }

  while (!stack.empty()) {
    Info info = stack.top();
    stack.pop();
    const FieldDescriptor* fd = std::get<0>(info);
    const Descriptor* desc = fd->message_type();
    if (desc) {
      for (int i = 0; i < desc->field_count(); i++) {
        const FieldDescriptor* field = desc->field(i);
        stack.push(Info(field, std::get<1>(info) + "." + field->name()));
      }
    } else {
      fds.push_back(std::get<0>(info));
      names.push_back(std::get<1>(info));
    }
  }

  return std::make_tuple(ColumnWriter(fds, names), Ok());
}

}  // namespace laminate
