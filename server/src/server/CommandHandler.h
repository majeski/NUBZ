#ifndef SERVER_COMMAND_HANDLER__H
#define SERVER_COMMAND_HANDLER__H

#include <thrift/server/TServer.h>

#include <utils/log.h>

#include <db/Database.h>

#include <repository/error/DuplicateName.h>
#include <repository/error/InUse.h>
#include <repository/error/InvalidData.h>

#include <communication/Server.h>

#include "io/InvalidInput.h"

namespace server {

class CommandHandler : public communication::ServerIf {
public:
    CommandHandler(db::Database &db);

    void setServer(apache::thrift::server::TServer *srv);

    virtual void shutdown() override;

    virtual int32_t ping(const communication::HelloMsg &msg) override;

    virtual void getMapImages(
        std::map<communication::FloorNum, communication::MapImage> &response) override;
    virtual void setMapImage(communication::MapImage &response,
                             const communication::SetMapImageRequest &request) override;
    virtual void removeFloor(const communication::FloorNum floor) override;

    virtual void getNewExhibits(communication::NewExhibitsResponse &response,
                                const communication::NewExhibitsRequest &request) override;
    virtual void createExhibit(communication::Exhibit &response,
                               const communication::CreateExhibitRequest &request) override;
    virtual void getAllExhibits(std::vector<communication::Exhibit> &response) override;
    virtual void setExhibitFrame(const communication::SetExhibitFrameRequest &request) override;
    virtual void updateExhibit(communication::Exhibit &response,
                               const communication::UpdateExhibitRequest &request) override;
    virtual void removeExhibit(const int32_t exhibitId) override;

    virtual void getCurrentExperiment(communication::CurrentExperimentResponse &response) override;
    virtual void createExperiment(const communication::CreateExperimentRequest &request) override;
    virtual void updateExperiment(const int32_t experimentId,
                                  const communication::CreateExperimentRequest &request) override;
    virtual void cloneExperiment(const communication::CloneExperimentRequest &request) override;
    virtual void getReadyExperiments(std::vector<communication::ExperimentInfo> &response) override;
    virtual void getFinishedExperiments(
        std::vector<communication::ExperimentInfo> &response) override;
    virtual void getActiveExperiment(communication::SingleExperimentInfo &response) override;
    virtual void getExperiment(communication::Experiment &response,
                               const int32_t experimentId) override;
    virtual void startExperiment(const int32_t experimentId) override;
    virtual void finishExperiment() override;
    virtual void removeExperiment(const int32_t experimentId) override;

    virtual int32_t getIdForNewReport() override;
    virtual void saveReport(const communication::RawReport &report) override;
    virtual void getAllReportsForExperiment(std::vector<communication::ReportInfo> &response,
                                            const int32_t experimentId) override;
    virtual void getExcelReport(communication::Filename &response, const int32_t reportId) override;
    virtual void getCombinedExcelReport(communication::Filename &response,
                                        const int32_t experimentId) override;

    virtual void createAction(communication::Action &response,
                              const communication::CreateActionRequest &request) override;
    virtual void getAllActions(std::vector<communication::Action> &response) override;
    virtual void removeAction(const int32_t actionId) override;

    virtual void getAllQuestions(communication::QuestionsList &response) override;

    virtual void createSimpleQuestion(
        communication::SimpleQuestion &response,
        const communication::CreateSimpleQuestionRequest &request) override;
    virtual void getAllSimpleQuestions(
        std::vector<communication::SimpleQuestion> &response) override;
    virtual void removeSimpleQuestion(const int32_t questionId) override;

    virtual void createMultipleChoiceQuestion(
        communication::MultipleChoiceQuestion &response,
        const communication::CreateMultipleChoiceQuestionRequest &request) override;
    virtual void getAllMultipleChoiceQuestions(
        std::vector<communication::MultipleChoiceQuestion> &response) override;
    virtual void removeMultipleChoiceQuestion(const int32_t questionId) override;

    virtual void createSortQuestion(
        communication::SortQuestion &response,
        const communication::CreateSortQuestionRequest &request) override;
    virtual void getAllSortQuestions(std::vector<communication::SortQuestion> &response) override;
    virtual void removeSortQuestion(const int32_t questionId) override;

private:
    db::Database &db;
    apache::thrift::server::TServer *srv;

    template <class F>
    std::result_of_t<F()> withExceptionTranslation(F &&f);
};

template <class F>
std::result_of_t<F()> CommandHandler::withExceptionTranslation(F &&f) {
    try {
        return f();
    } catch (server::io::InvalidInput &e) {
        LOG(INFO) << "InvalidInput: " << e.what();
        LOG(INFO) << "Command aborted";
        throw e.toThrift();
    } catch (repository::InvalidData &e) {
        LOG(INFO) << "InvalidInput: " << e.what();
        LOG(INFO) << "Command aborted";
        throw communication::InvalidData{};
    } catch (repository::DuplicateName &e) {
        LOG(INFO) << "DuplicateName: " << e.what();
        LOG(INFO) << "Command aborted";
        throw communication::DuplicateName{};
    } catch (repository::InUse &e) {
        LOG(INFO) << "InUse: " << e.what();
        LOG(INFO) << "Command aborted";
        throw communication::ElementInUse{};
    } catch (std::exception &e) {
        LOG(INFO) << "InternalError: " << e.what();
        LOG(INFO) << "Command aborted";
        throw communication::InternalError{};
    } catch (...) {
        LOG(INFO) << "InternalError";
        LOG(INFO) << "Command aborted";
        throw communication::InternalError();
    }
}
}

#endif