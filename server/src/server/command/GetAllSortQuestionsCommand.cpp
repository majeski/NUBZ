#include <repository/SortQuestions.h>

#include <server/io/utils.h>

#include "GetAllSortQuestionsCommand.h"

namespace server {
namespace command {

GetAllSortQuestionsCommand::GetAllSortQuestionsCommand(db::Database &db) : db(db) {
}

std::vector<io::output::SortQuestion> GetAllSortQuestionsCommand::operator()() {
    auto repoQuestions = db.execute([&](db::DatabaseSession &session) {
        auto repo = repository::SortQuestions{session};
        return repo.getAll();
    });

    auto result = io::repoToIO<io::output::SortQuestion>(repoQuestions);
    std::sort(result.begin(), result.end());
    return result;
}
}
}