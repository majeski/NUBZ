#include <utils/fp_algorithm.h>

#include <db/table/Exhibits.h>

#include "Counters.h"
#include "DefaultRepo.h"
#include "Exhibits.h"
#include "MapImages.h"
#include "error/DuplicateName.h"
#include "error/InvalidData.h"

namespace repository {

using Table = db::table::Exhibits;
using Impl = repository::detail::DefaultRepoID<Table>;

namespace {
struct OptFrame {
    OptFrame(const boost::optional<Exhibits::Exhibit::Frame> &frame);

    boost::optional<std::int32_t> x;
    boost::optional<std::int32_t> y;
    boost::optional<std::int32_t> width;
    boost::optional<std::int32_t> height;
    boost::optional<std::int32_t> floor;
};

Table::Sql::in_t toDB(const Exhibits::Exhibit &exhibit);
Exhibits::Exhibit fromDB(const Table::Sql::out_t &exhibit);
}

Exhibits::Exhibits(db::DatabaseSession &session) : session(session) {
}

void Exhibits::incReferenceCount(std::int32_t ID) {
    updateReferenceCount(ID, 1);
}

void Exhibits::decReferenceCount(std::int32_t ID) {
    updateReferenceCount(ID, -1);
}

void Exhibits::updateReferenceCount(std::int32_t ID, std::int32_t diff) {
    auto get = db::sql::Select<Table::FieldID, Table::FieldRefCount>{}.where(Table::ID == ID);
    if (auto exhibit = session.getResult(get)) {
        auto count = std::get<Table::FieldRefCount>(exhibit.value()).value + diff;
        assert(count >= 0);
        auto sql = Table::Sql::update().where(Table::ID == ID).set(Table::RefCount, count);
        session.execute(sql);
    } else {
        throw InvalidData{"incorrect exhibit ID"};
    }
}

void Exhibits::refresh() {
    auto sql = Table::Sql::del().where(Table::IsDeleted == true && Table::RefCount == 0);
    session.execute(sql);
}

std::vector<std::int32_t> Exhibits::getAllIDs() {
    auto sql =
        db::sql::Select<Table::FieldID, Table::FieldIsDeleted>{}.where(Table::IsDeleted == false);
    auto dbTuples = session.getResults(sql);

    auto result = std::vector<std::int32_t>{};
    for (auto &dbTuple : dbTuples) {
        result.push_back(std::get<Table::FieldID>(dbTuple).value);
    }
    return result;
}

Exhibit Exhibits::get(std::int32_t ID) {
    if (auto res = getOpt(ID)) {
        return res.value();
    } else {
        throw InvalidData{"incorrect exhibit ID"};
    }
}

boost::optional<Exhibits::Exhibit> Exhibits::getOpt(std::int32_t ID) {
    auto sql = Table::Sql::select().where(Table::ID == ID && Table::IsDeleted == false);
    if (auto res = session.getResult(sql)) {
        return fromDB(res.value());
    } else {
        return {};
    }
}

std::vector<Exhibits::Exhibit> Exhibits::getAllWithDeleted() {
    auto sql = Table::Sql::select();
    auto result = std::vector<Exhibits::Exhibit>{};
    utils::transform(session.getResults(sql), result, fromDB);
    return result;
}

// ignores deleted exhibits
std::vector<Exhibits::Exhibit> Exhibits::getAll() {
    auto sql = Table::Sql::select().where(Table::IsDeleted == false);
    auto result = std::vector<Exhibits::Exhibit>{};
    utils::transform(session.getResults(sql), result, fromDB);
    return result;
}

// ignores deleted exhibits
std::vector<Exhibit> Exhibits::getAllNewerThan(std::int32_t version) {
    if (version < getLastDeletedVersion()) {
        assert(false &&
               "there is no way to include deleted exhibits in an update for this version");
    }

    auto sql = Table::Sql::select().where(Table::Version > version && Table::IsDeleted == false);
    auto dbTuples = session.getResults(sql);

    auto result = std::vector<Exhibit>{};
    utils::transform(dbTuples, result, fromDB);
    return result;
}

void Exhibits::remove(std::int32_t ID) {
    checkID(ID);

    auto sql = Table::Sql::update().where(Table::ID == ID).set(Table::IsDeleted, true);
    session.execute(sql);

    auto curVersion = Counters{session}.increment(CounterType::LastExhibitVersion);
    Counters{session}.set(CounterType::LastDeletedExhibitVersion, curVersion);
}

void Exhibits::insert(Exhibits::Exhibit *exhibit) {
    assert(exhibit);

    checkName(exhibit->name);
    checkRgbHex(exhibit->rgbHex);
    if (exhibit->frame) {
        checkFrame(exhibit->frame.value());
    }

    exhibit->version = Counters{session}.increment(CounterType::LastExhibitVersion);
    exhibit->ID = Impl::insert(session, toDB(*exhibit));
}

void Exhibits::setRgbHex(std::int32_t ID, std::int32_t newRgbHex) {
    checkID(ID);
    checkRgbHex(newRgbHex);

    auto sql = Table::Sql::update().set(Table::RgbHex, newRgbHex).where(Table::ID == ID);
    session.execute(sql);
}

void Exhibits::setFrame(std::int32_t ID, const boost::optional<Exhibit::Frame> &newFrame) {
    checkID(ID);
    if (newFrame) {
        checkFrame(newFrame.value());
    }

    auto frame = OptFrame{newFrame};
    auto version = Counters{session}.increment(CounterType::LastExhibitVersion);
    auto sql = Table::Sql::update()
                   .where(Table::ID == ID)
                   .set(Table::Version, version)
                   .set(Table::FrameX, frame.x)
                   .set(Table::FrameY, frame.y)
                   .set(Table::FrameWidth, frame.width)
                   .set(Table::FrameHeight, frame.height)
                   .set(Table::FrameFloor, frame.floor);
    session.execute(sql);
}

void Exhibits::resetFrames(std::int32_t floor) {
    using namespace db::sql;
    auto version = Counters{session}.increment(CounterType::LastExhibitVersion);
    auto sql = Table::Sql::update()
                   .where(Table::FrameFloor == floor)
                   .set(Table::Version, version)
                   .set(Table::FrameX, Null)
                   .set(Table::FrameY, Null)
                   .set(Table::FrameWidth, Null)
                   .set(Table::FrameHeight, Null)
                   .set(Table::FrameFloor, Null);
    session.execute(sql);
}

std::int32_t Exhibits::getCurrentVersion() {
    return Counters{session}.get(repository::CounterType::LastExhibitVersion);
}

std::int32_t Exhibits::getLastDeletedVersion() {
    return Counters{session}.get(repository::CounterType::LastDeletedExhibitVersion);
}

void Exhibits::checkID(std::int32_t ID) {
    auto sql = db::sql::Select<Table::FieldID, Table::FieldIsDeleted>{}.where(
        Table::ID == ID && Table::IsDeleted == false);
    if (!session.getResult(sql)) {
        throw InvalidData{"incorrect exhibit ID"};
    }
}

void Exhibits::checkFrame(const Exhibit::Frame &frame) {
    if (auto mapImage = MapImages{session}.get(frame.floor)) {
        if (frame.x + frame.width > mapImage.value().width ||
            frame.y + frame.height > mapImage.value().height || frame.x < 0 || frame.y < 0) {
            throw InvalidData{"incorrect exhibit frame - out of bounds"};
        }
    } else {
        throw InvalidData{"incorrect exhibit frame - given floor doesn't exist"};
    }
}

void Exhibits::checkName(const std::string &name) {
    if (name.empty()) {
        throw InvalidData{"exhibit name cannot be empty"};
    }

    auto sql = db::sql::Select<Table::FieldName, Table::FieldIsDeleted>{}.where(
        Table::Name == name && Table::IsDeleted == false);
    if (session.getResult(sql)) {
        throw DuplicateName{};
    }
}

void Exhibits::checkRgbHex(std::int32_t rgbHex) {
    if ((rgbHex & 0x00FFFFFF) != rgbHex) {
        throw InvalidData{"incorrect rgbHex value"};
    }
}

namespace {
OptFrame::OptFrame(const boost::optional<Exhibits::Exhibit::Frame> &optFrame) {
    if (optFrame) {
        auto frame = optFrame.value();
        x = frame.x;
        y = frame.y;
        width = frame.width;
        height = frame.height;
        floor = frame.floor;
    }
}

Table::Sql::in_t toDB(const Exhibits::Exhibit &exhibit) {
    auto frame = OptFrame{exhibit.frame};

    return std::make_tuple(Table::FieldName{exhibit.name},
                           Table::FieldVersion{exhibit.version},
                           Table::FieldRgbHex{exhibit.rgbHex},
                           Table::FieldFrameX{frame.x},
                           Table::FieldFrameY{frame.y},
                           Table::FieldFrameWidth{frame.width},
                           Table::FieldFrameHeight{frame.height},
                           Table::FieldFrameFloor{frame.floor});
}

Exhibits::Exhibit fromDB(const Table::Sql::out_t &exhibit) {
    auto result = Exhibits::Exhibit{};
    result.ID = std::get<Table::FieldID>(exhibit).value;
    result.name = std::get<Table::FieldName>(exhibit).value;
    result.version = std::get<Table::FieldVersion>(exhibit).value;
    result.rgbHex = std::get<Table::FieldRgbHex>(exhibit).value;

    auto x = std::get<Table::FieldFrameX>(exhibit).value;
    auto y = std::get<Table::FieldFrameY>(exhibit).value;
    auto width = std::get<Table::FieldFrameWidth>(exhibit).value;
    auto height = std::get<Table::FieldFrameHeight>(exhibit).value;
    auto floor = std::get<Table::FieldFrameFloor>(exhibit).value;
    if (x && y && width && height && floor) {
        auto frame = Exhibits::Exhibit::Frame{};
        frame.x = x.value();
        frame.y = y.value();
        frame.width = width.value();
        frame.height = height.value();
        frame.floor = floor.value();
        result.frame = frame;
    }

    return result;
}
}
}
