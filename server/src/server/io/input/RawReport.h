#ifndef SERVER_IO_INPUT__RAW_REPORT__H
#define SERVER_IO_INPUT__RAW_REPORT__H

#include <cstdint>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <communication/structs_types.h>

#include <utils/TimePoint.h>

#include <repository/Reports.h>

namespace server {
namespace io {
namespace input {

struct RawReport {
    struct Event {
        struct Time {
            Time(const communication::Time &thrift);
            ::utils::TimePoint toRepo() const;

            const std::int32_t hour;
            const std::int32_t min;
            const std::int32_t sec;
        };

        Event(const communication::RawReportEvent &thrift);
        repository::Report::Event toRepo() const;

        boost::optional<std::int32_t> exhibitId;
        const Time startTime;
        const std::int32_t durationInSecs;
        const std::vector<std::int32_t> actions;
    };

    struct SurveyAnswers {
        using SimpleQuestionAnswer = std::string;
        using MultipleChoiceQuestionAnswer = std::vector<std::int32_t>;
        using SortQuestionAnswer = std::vector<std::int32_t>;

        SurveyAnswers(const communication::SurveyAnswers &thrift);
        repository::Report::SurveyAns toRepo() const;

        std::vector<SimpleQuestionAnswer> simpleQuestionsAnswers;
        std::vector<MultipleChoiceQuestionAnswer> multipleChoiceQuestionsAnswers;
        std::vector<SortQuestionAnswer> sortQuestionsAnswers;
    };

    RawReport(const communication::RawReport &thrift);
    repository::Report toRepo() const;

    std::int32_t experimentId;
    std::int32_t ID;

    SurveyAnswers answersBefore;
    SurveyAnswers answersAfter;

    std::vector<Event> history;
};
}
}
}

#endif