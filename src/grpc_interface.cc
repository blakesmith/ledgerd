#include <memory>
#include <string>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>

#include "ledgerd_consumer.h"
#include "grpc_interface.h"


namespace ledgerd {
GrpcInterface::GrpcInterface(LedgerdService& ledgerd_service)
    : ledgerd_service_(ledgerd_service) { }

LedgerdStatus GrpcInterface::translate_status(ledger_status rc) {
    switch(rc) {
        case ::LEDGER_OK:
            return LedgerdStatus::OK;
        case ::LEDGER_NEXT:
            return LedgerdStatus::NEXT;
        case ::LEDGER_ERR_GENERAL:
            return LedgerdStatus::ERR_GENERAL;
        case ::LEDGER_ERR_MEMORY:
            return LedgerdStatus::ERR_MEMORY;
        case ::LEDGER_ERR_MKDIR:
            return LedgerdStatus::ERR_MKDIR;
        case ::LEDGER_ERR_ARGS:
            return LedgerdStatus::ERR_ARGS;
        case ::LEDGER_ERR_BAD_TOPIC:
            return LedgerdStatus::ERR_BAD_TOPIC;
        case ::LEDGER_ERR_BAD_PARTITION:
            return LedgerdStatus::ERR_BAD_PARTITION;
        case ::LEDGER_ERR_BAD_META:
            return LedgerdStatus::ERR_BAD_META;
        case ::LEDGER_ERR_BAD_LOCKFILE:
            return LedgerdStatus::ERR_BAD_LOCKFILE;
        case ::LEDGER_ERR_IO:
            return LedgerdStatus::ERR_IO;
        case ::LEDGER_ERR_POSITION_NOT_FOUND:
            return LedgerdStatus::ERR_POSITION_NOT_FOUND;
    }

    return LedgerdStatus::OK;
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

static ledger_consume_status stream_f(ledger_consumer_ctx* ctx,
                                      ledger_message_set* messages,
                                      void* data) {
    auto writer = static_cast<grpc::ServerWriter<LedgerdMessageSet>*>(data);
    LedgerdMessageSet message_set;
    message_set.set_next_id(messages->next_id);
    for(int i = 0; i < messages->nmessages; i++) {
        LedgerdMessage* message = message_set.add_messages();
        message->set_id(messages->messages[i].id);
        message->set_data(messages->messages[i].data,
                          messages->messages[i].len);
    }
    writer->Write(message_set);
}

grpc::Status GrpcInterface::StreamPartition(grpc::ServerContext *context, const StreamPartitionRequest* request, grpc::ServerWriter<LedgerdMessageSet>* writer) {
    ledger_status rc;
    ledger_consumer_options consumer_options;
    const std::string position_key = request->position_settings().position_key();

    ledger_init_consumer_options(&consumer_options);
    consumer_options.read_chunk_size = request->read_chunk_size();
    if(request->position_settings().behavior() == PositionBehavior::STORE) {
        consumer_options.position_behavior = ::LEDGER_STORE;
        consumer_options.position_key = position_key.c_str();
    } else {
        consumer_options.position_behavior = ::LEDGER_FORGET;
    }

    try {
        Consumer consumer(stream_f, &consumer_options, writer);
        rc = ledgerd_service_.StartConsumer(&consumer,
                                            request->topic_name(),
                                            request->partition_num(),
                                            request->start_id());
        if(rc != ::LEDGER_OK) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Something went wrong");
        }

        consumer.Wait();
    } catch (std::invalid_argument& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }

    return grpc::Status::OK;
}

}
