#include <gtest/gtest.h>
#include <cstring>
#include <Fw/Test/UnitTest.hpp>
#include <Fw/Types/MallocAllocator.hpp>
#include <Fw/Types/String.hpp>
#include <Os/File.hpp>
#include <Os/FileSystem.hpp>
#include <fprime-baremetal/Os/Baremetal/MicroFs/MicroFs.hpp>
#include "Tester.hpp"

#define FULL_TEST
#define NUKE_TEST
#define OFF_NOMINAL
#define NEW_TEST
#define SIM_FILE_TEST

#ifdef FULL_TEST

TEST(Initialization, InitTest) {
    Os::Tester tester;
    tester.InitTest();
}

TEST(FileOps, OpenWriteReadTest) {
    Os::Tester tester;
    tester.OpenWriteReadTest();
}

TEST(FileOps, OpenWriteTwiceReadOnceTest) {
    Os::Tester tester;
    tester.OpenWriteTwiceReadOnceTest();
}

TEST(FileOps, OpenWriteOnceReadTwiceTest) {
    Os::Tester tester;
    tester.OpenWriteOnceReadTwiceTest();
}

TEST(FileSystemOps, ListTest) {
    Os::Tester tester;
    tester.ListTest();
}

TEST(FileSystemOps, OpenFreeSpaceTest) {
    Os::Tester tester;
    tester.OpenFreeSpaceTest();
}

TEST(FileOps, OpenStressTest) {
    Os::Tester tester;
    tester.OpenStressTest();
}

TEST(FileOps, FileSizeTest) {
    Os::Tester tester;
    tester.FileSizeTest();
}

TEST(FileOps, ReWriteTest) {
    Os::Tester tester;
    tester.ReWriteTest();
}

TEST(FileOps, DirectoryTest) {
    Os::Tester tester;
    tester.DirectoryTest();
}

TEST(FileOps, MoveTest) {
    Os::Tester tester;
    tester.MoveTest();
}

TEST(FileOps, SeekTest) {
    Os::Tester tester;
    tester.SeekTest();
}

TEST(FileOps, CrcTest) {
    Os::Tester tester;
    tester.CrcTest();
}

TEST(FileOps, OddTests) {
    Os::Tester tester;
    tester.OddTests();
}

TEST(FileOps, CopyTest) {
    Os::Tester tester;
    tester.CopyTest();
}

TEST(FileOps, AppendTest) {
    Os::Tester tester;
    tester.AppendTest();
}

#endif

#ifdef NUKE_TEST
TEST(FileOps, NukeTest) {
    Os::Tester tester;
    tester.NukeTest();
}
#endif

#ifdef OFF_NOMINAL
TEST(FileOps, OffNominalTests) {
    Os::Tester tester;
    tester.OffNominalTests();
}
#endif

#ifdef SIM_FILE_TEST
TEST(FileOps, SimFileTest) {
    Os::Tester tester;
    tester.SimFileTest();
}
#endif

#ifdef NEW_TEST
TEST(FileOps, NewTest) {
    Os::Tester tester;
    tester.NewTest();
}
#endif

TEST(AliasOps, AliasResolvesToSlotTest) {
    Fw::MallocAllocator alloc;
    Os::Baremetal::MicroFs::MicroFsConfig cfg;
    Os::Baremetal::MicroFs::MicroFsSetCfgBins(cfg, 1);
    Os::Baremetal::MicroFs::MicroFsAddBin(cfg, 0, 128, 2);
    Os::Baremetal::MicroFs::MicroFsInit(cfg, 0, alloc);

    ASSERT_EQ(Os::Baremetal::MicroFs::registerAlias("PrmDb.dat", "/bin0/file1"),
              Os::Baremetal::MicroFs::Status::VALID);
    // duplicate registration is rejected
    ASSERT_EQ(Os::Baremetal::MicroFs::registerAlias("PrmDb.dat", "/bin0/file1"),
              Os::Baremetal::MicroFs::Status::INVALID);

    // write via the alias
    U8 writeData[4] = {1, 2, 3, 4};
    FwSizeType writeSize = sizeof(writeData);
    Os::File writeFile;
    ASSERT_EQ(writeFile.open("PrmDb.dat", Os::File::OPEN_CREATE), Os::File::Status::OP_OK);
    ASSERT_EQ(writeFile.write(writeData, writeSize), Os::File::Status::OP_OK);
    ASSERT_EQ(writeSize, sizeof(writeData));
    writeFile.close();

    // read back via the canonical slot path to confirm it's the same slot
    U8 readData[4] = {0};
    FwSizeType readSize = sizeof(readData);
    Os::File readFile;
    ASSERT_EQ(readFile.open("/bin0/file1", Os::File::OPEN_READ), Os::File::Status::OP_OK);
    ASSERT_EQ(readFile.read(readData, readSize), Os::File::Status::OP_OK);
    readFile.close();
    ASSERT_EQ(readSize, sizeof(writeData));
    ASSERT_EQ(memcmp(writeData, readData, sizeof(writeData)), 0);

    // an unregistered human-readable name is still rejected
    Os::File unknownFile;
    ASSERT_EQ(unknownFile.open("Nope.dat", Os::File::OPEN_READ), Os::File::Status::DOESNT_EXIST);

    Os::Baremetal::MicroFs::MicroFsCleanup(0, alloc);
}

TEST(AliasOps, ColdBootThenSaveThenReadTest) {
    Fw::MallocAllocator alloc;
    Os::Baremetal::MicroFs::MicroFsConfig cfg;
    Os::Baremetal::MicroFs::MicroFsSetCfgBins(cfg, 1);
    Os::Baremetal::MicroFs::MicroFsAddBin(cfg, 0, 128, 2);
    Os::Baremetal::MicroFs::MicroFsInit(cfg, 0, alloc);

    ASSERT_EQ(Os::Baremetal::MicroFs::registerAlias("PrmDb2.dat", "/bin0/file0"),
              Os::Baremetal::MicroFs::Status::VALID);

    // Cold boot: nothing has ever been written to this (volatile) slot yet.
    Os::File coldOpen;
    ASSERT_EQ(coldOpen.open("PrmDb2.dat", Os::File::OPEN_READ), Os::File::Status::DOESNT_EXIST);

    // Simulate a PRM_SAVE_FILE: write the file once via the alias.
    U8 saveData[3] = {0xA5, 0x01, 0x02};
    FwSizeType saveSize = sizeof(saveData);
    Os::File saveFile;
    ASSERT_EQ(saveFile.open("PrmDb2.dat", Os::File::OPEN_WRITE), Os::File::Status::OP_OK);
    ASSERT_EQ(saveFile.write(saveData, saveSize), Os::File::Status::OP_OK);
    saveFile.close();

    // Now a read-open through the same alias succeeds, as it would on the next readParamFile().
    U8 loadData[3] = {0};
    FwSizeType loadSize = sizeof(loadData);
    Os::File loadFile;
    ASSERT_EQ(loadFile.open("PrmDb2.dat", Os::File::OPEN_READ), Os::File::Status::OP_OK);
    ASSERT_EQ(loadFile.read(loadData, loadSize), Os::File::Status::OP_OK);
    loadFile.close();
    ASSERT_EQ(loadSize, sizeof(saveData));
    ASSERT_EQ(memcmp(saveData, loadData, sizeof(saveData)), 0);

    Os::Baremetal::MicroFs::MicroFsCleanup(0, alloc);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
