#include <repository/Counters.h>
#include <repository/Exhibits.h>

#include <server/io/InvalidInput.h>
#include <server/utils/InputChecker.h>

#include "SetExhibitFrameCommand.h"

namespace server {
namespace command {

SetExhibitFrameCommand::SetExhibitFrameCommand(db::Database &db) : db(db) {
}

// function can only alter existing exhibit frame
// if exhibit has no floor InvalidData will be thrown
void SetExhibitFrameCommand::operator()(const io::input::SetExhibitFrameRequest &input) {
    db.execute([&](db::DatabaseSession &session) {
        auto countersRepo = repository::Counters{session};
        auto version = countersRepo.increment(repository::CounterType::LastExhibitVersion);

        auto exhibitsRepo = repository::Exhibits{session};
        auto exhibit = exhibitsRepo.get(input.exhibitId).value();
        auto repoFrame = exhibit.frame.value();
        repoFrame.x = input.frame.x;
        repoFrame.y = input.frame.y;
        repoFrame.width = input.frame.size.width;
        repoFrame.height = input.frame.size.height;

        exhibitsRepo.setVersion(exhibit.ID, version);
        exhibitsRepo.setFrame(exhibit.ID, repoFrame);
    });
}
}
}