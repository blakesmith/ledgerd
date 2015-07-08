#include <memory>
#include <string>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>

#include "grpc_interface.h"


namespace ledgerd {
GrpcInterface::GrpcInterface(LedgerdService& ledgerd_service)
    : ledgerd_service_(ledgerd_service) { }

LedgerdStatus GrpcInterface::translate_status(ledger_status rc) {
    switch(rc) {
        case ::LEDGER_OK:
            return LedgerdStatus::LEDGER_OK;
        case ::LEDGER_NEXT:
            return LedgerdStatus::LEDGER_NEXT;
        case ::LEDGER_ERR_GENERAL:
            return LedgerdStatus::LEDGER_ERR_GENERAL;
        case ::LEDGER_ERR_MEMORY:
            return LedgerdStatus::LEDGER_ERR_MEMORY;
        case ::LEDGER_ERR_MKDIR:
            return LedgerdStatus::LEDGER_ERR_MKDIR;
        case ::LEDGER_ERR_ARGS:
            return LedgerdStatus::LEDGER_ERR_ARGS;
        case ::LEDGER_ERR_BAD_TOPIC:
            return LedgerdStatus::LEDGER_ERR_BAD_TOPIC;
        case ::LEDGER_ERR_BAD_PARTITION:
            return LedgerdStatus::LEDGER_ERR_BAD_PARTITION;
        case ::LEDGER_ERR_BAD_META:
            return LedgerdStatus::LEDGER_ERR_BAD_META;
        case ::LEDGER_ERR_BAD_LOCKFILE:
            return LedgerdStatus::LEDGER_ERR_BAD_LOCKFILE;
        case ::LEDGER_ERR_IO:
            return LedgerdStatus::LEDGER_ERR_IO;
        case ::LEDGER_ERR_POSITION_NOT_FOUND:
            return LedgerdStatus::LEDGER_ERR_POSITION_NOT_FOUND;
    }

    return LedgerdStatus::LEDGER_OK;
}

grpc::Status GrpcInterface::Ping(grpc::ServerContext *context, const PingRequest *req,
                                 PingResponse *resp) {
    resp->set_pong("pong");
    return grpc::Status::OK;
}

grpc::Status GrpcInterface::OpenTopic(grpc::ServerContext *context, const OpenTopicRequest *req,
                                      LedgerdResponse *resp) {
    ledger_topic_options topic_options;
    ledger_status rc;

    ledger_topic_options_init(&topic_options);

    if(req->has_options()) {
        const TopicOptions& opts = req->options();
        topic_options.drop_corrupt = opts.drop_corrupt();
        topic_options.journal_max_size_bytes = opts.journal_max_size_bytes();
    }

    rc = ledgerd_service_.OpenTopic(req->name(), req->partition_count(), &topic_options);
    resp->set_status(translate_status(rc));
    if(rc != ::LEDGER_OK) {
        // TODO: I REALLY need a reliable way to get error messages out
        resp->set_error_message("Something went wrong");
    }

    return grpc::Status::OK;
}

grpc::Status GrpcInterface::WritePartition(grpc::ServerContext *context, const WritePartitionRequest *req,
                            WriteResponse *resp) {
    ledger_status rc;
    ledger_write_status status;

    rc = ledgerd_service_.WritePartition(req->topic_name(), req->partition_num(), req->data(), &status);
    resp->mutable_ledger_response()->set_status(translate_status(rc));
    if(rc != ::LEDGER_OK) {
        resp->mutable_ledger_response()->set_error_message("Something went wrong");
        return grpc::Status::OK;
    }
    resp->set_message_id(status.message_id);
    resp->set_partition_num(status.partition_num);

    return grpc::Status::OK;
}

grpc::Status GrpcInterface::ReadPartition(grpc::ServerContext *context, const ReadPartitionRequest *req,
                                          ReadResponse *resp) {
    ledger_status rc;
    ledger_message_set messages;

    rc = ledgerd_service_.ReadPartition(req->topic_name(), req->partition_num(), req->start_id(), req->nmessages(), &messages);
    resp->mutable_ledger_response()->set_status(translate_status(rc));
    if(rc != ::LEDGER_OK) {
        resp->mutable_ledger_response()->set_error_message("Something went wrong");
        return grpc::Status::OK;
    }
    LedgerdMessageSet* message_set = resp->mutable_messages();
    message_set->set_next_id(messages.next_id);

    for(int i = 0; i < messages.nmessages; ++i) {
        LedgerdMessage* message = message_set->add_messages();
        message->set_id(messages.messages[i].id);
        message->set_data(messages.messages[i].data, messages.messages[i].len);
    }

    ledger_message_set_free(&messages);

    return grpc::Status::OK;
}

}
