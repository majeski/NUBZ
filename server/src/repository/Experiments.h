#ifndef REPOSITORY__EXPERIMENTS__H
#define REPOSITORY__EXPERIMENTS__H

#include <cstdint>
#include <string>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/optional.hpp>

#include <db/DatabaseSession.h>

#include "Actions.h"
#include "MultipleChoiceQuestions.h"
#include "SimpleQuestions.h"
#include "SortQuestions.h"

namespace repository {

class Experiments {
public:
    struct _Experiment {
        enum QuestionType : std::int32_t { Simple = 0, MultipleChoice = 1, Sort = 2 };

        std::int32_t ID;
        std::string name;
        boost::optional<boost::gregorian::date> startDate;
        boost::optional<boost::gregorian::date> finishDate;
    };

    struct Experiment : _Experiment {
        struct Survey {
            using QuestionType = _Experiment::QuestionType;

            // defines questions order
            std::vector<QuestionType> typesOrder;

            std::vector<SimpleQuestion> simpleQuestions;
            std::vector<MultipleChoiceQuestion> multipleChoiceQuestions;
            std::vector<SortQuestion> sortQuestions;
        };

        std::vector<Action> actions;
        std::vector<Action> breakActions;

        Survey surveyBefore;
        Survey surveyAfter;
    };

    // contains ids instead of whole objects
    struct LazyExperiment : _Experiment {
        struct Survey {
            using QuestionType = _Experiment::QuestionType;

            // defines questions order
            std::vector<QuestionType> typesOrder;

            // unique, foreign keys
            std::vector<std::int32_t> simpleQuestions;

            // unique, foreign keys
            std::vector<std::int32_t> multipleChoiceQuestions;

            // unique, foreign keys
            std::vector<std::int32_t> sortQuestions;
        };

        // unique, foreign keys
        std::vector<std::int32_t> actions;

        // unique, foreign keys
        std::vector<std::int32_t> breakActions;

        Survey surveyBefore;
        Survey surveyAfter;
    };

    Experiments(db::DatabaseSession &session);

    boost::optional<Experiment> get(std::int32_t ID);
    boost::optional<LazyExperiment> getLazy(std::int32_t ID);

    // may return null in case of incorrect id or id of finished experiment (it's impossible to
    // activate finished experiment)
    boost::optional<Experiment> start(std::int32_t ID);

    boost::optional<Experiment> getActive();
    void finishActive();

    std::vector<Experiment> getAllReady();
    std::vector<Experiment> getAllFinished();

    // ID, startDate and finishDate will be saved in the given struct
    // may throw InvalidData
    void insert(LazyExperiment *experiment);

private:
    enum State : std::int32_t { Ready = 0, Active = 1, Finished = 2 };

    std::vector<Experiment> getAllWithState(State state);

    void checkActions(const Experiments::LazyExperiment &experiment);

    // throws InvalidData in case of duplicated questions, incorrect id or invalid types order
    void checkSurvey(const LazyExperiment::Survey &survey);

    // throws InvalidData in case of not existing or duplicated id
    void checkIds(const std::unordered_set<std::int32_t> &existing,
                  const std::vector<std::int32_t> &choosen) const;

    void checkForDuplicates(std::vector<std::int32_t> ids) const;

    db::DatabaseSession &session;
};

using Experiment = Experiments::Experiment;
}

#endif