#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE AudioRanker
#include <boost/test/unit_test.hpp>
#include <boost/throw_exception.hpp>

#include "AudioRanker.h"

void boost::throw_exception(std::exception const & e)
{
    printf("exception: %s", e.what());
}

class RankRecorder : public owt_base::AudioRanker::Visitor {
public:
    void onRankChange(
        std::vector<std::pair<std::string, std::string>> updates) override
    {
        printf("---updates begin\n");
        for (auto& pair : updates) {
            printf("%s - %s\n", pair.first.c_str(), pair.second.c_str());
        }
        printf("---updates end\n");

        m_count++;
        boost::mutex::scoped_lock lock(m_mutex);
        m_data = updates;
    }

    std::vector<std::pair<std::string, std::string>> data()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_data;
    }
    int count() { return m_count; }
private:
    boost::mutex m_mutex;
    std::atomic<int> m_count{0};
    std::vector<std::pair<std::string, std::string>> m_data;
};

class TestSource : public owt_base::FrameSource {
public:
    void generateFrame(const owt_base::Frame& frame)
    {
        deliverFrame(frame);
    }
};

class TestDestination: public owt_base::FrameDestination {
public:
    TestDestination() : m_count(0) {}

    void onFrame(const owt_base::Frame&) override
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_count++;
    }
    int count()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_count;
    }
private:
    boost::mutex m_mutex;
    int m_count;
};

struct Recorder
{
    RankRecorder recorder;
    TestSource src1, src2, src3;
    TestDestination dest1, dest2, dest3;
    owt_base::Frame frame;

    Recorder()
    {
        memset(&frame, 0, sizeof(frame));
        frame.format = owt_base::FRAME_FORMAT_OPUS;
        frame.additionalInfo.audio.voice = 1;
    }
};

BOOST_FIXTURE_TEST_SUITE(Ranker, Recorder)

BOOST_AUTO_TEST_CASE(OutputAndInputInit)
{
    owt_base::AudioRanker ranker(&recorder, false, 10);
    ranker.addOutput(&dest1);
    BOOST_CHECK(recorder.data().empty());
    BOOST_CHECK(recorder.count() == 0);
    ranker.addInput(&src1, "src1", "owner1");
    ranker.addInput(&src2, "src2", "owner2");

    // Output dest1 should be linked with first input src1
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    BOOST_CHECK(recorder.data().front().first == "src1");
    BOOST_CHECK(recorder.count() == 1);

    BOOST_CHECK(dest1.count() == 0);
    frame.additionalInfo.audio.audioLevel = 50;
    src1.generateFrame(frame);

    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    BOOST_CHECK(dest1.count() == 1);
}

BOOST_AUTO_TEST_CASE(OutputAndInputSwitchOne)
{
    owt_base::AudioRanker ranker(&recorder, false, 10);
    ranker.addOutput(&dest1);
    ranker.addInput(&src1, "src1", "owner1");
    ranker.addInput(&src2, "src2", "owner2");

    // Output dest1 should be linked with first input src1
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    BOOST_CHECK(recorder.data().front().first == "src1");
    BOOST_CHECK(dest1.count() == 0);

    // Still link src1 when src2 deliver frame
    frame.additionalInfo.audio.audioLevel = 40;
    src2.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    BOOST_CHECK(recorder.data().front().first == "src2");
    BOOST_CHECK(dest1.count() == 0);

    // After level 40 frame, dest1 should be linked with src2
    frame.additionalInfo.audio.audioLevel = 30;
    src2.generateFrame(frame);
    BOOST_CHECK(dest1.count() == 1);

    // Level 50 frame on src1 won't trigger update
    frame.additionalInfo.audio.audioLevel = 50;
    src1.generateFrame(frame);
    BOOST_CHECK(dest1.count() == 1);

    // Level 60 frame let dest1 link src1
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    frame.additionalInfo.audio.audioLevel = 60;
    src2.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    BOOST_CHECK(recorder.data().front().first == "src1");
    BOOST_CHECK(dest1.count() == 2);

    frame.additionalInfo.audio.audioLevel = 50;
    src1.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    BOOST_CHECK(dest1.count() == 3);

}

BOOST_AUTO_TEST_CASE(TwoOutputs)
{
    std::string top1, top2;
    owt_base::AudioRanker ranker(&recorder, false, 0);
    ranker.addOutput(&dest1);
    ranker.addOutput(&dest2);
    ranker.addInput(&src1, "src1", "owner1");
    ranker.addInput(&src2, "src2", "owner2");
    ranker.addInput(&src3, "src3", "owner3");
    boost::this_thread::sleep_for(boost::chrono::milliseconds(20));

    frame.additionalInfo.audio.audioLevel = 60;
    src1.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 70;
    src2.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 80;
    src3.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
    // Top 2 should be src1, src2
    BOOST_CHECK(recorder.data().size() == 2);
    top1 = std::min(recorder.data()[0].first, recorder.data()[1].first);
    top2 = std::max(recorder.data()[0].first, recorder.data()[1].first);
    BOOST_CHECK(top1 == "src1");
    BOOST_CHECK(top2 == "src2");

    frame.additionalInfo.audio.audioLevel = 60;
    src1.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 50;
    src2.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 40;
    src3.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
    // Top 2 should be src3, src2
    BOOST_CHECK(recorder.data().size() == 2);
    top1 = std::min(recorder.data()[0].first, recorder.data()[1].first);
    top2 = std::max(recorder.data()[0].first, recorder.data()[1].first);
    BOOST_CHECK(top1 == "src2");
    BOOST_CHECK(top2 == "src3");

    frame.additionalInfo.audio.audioLevel = 30;
    src1.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 50;
    src2.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 40;
    src3.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
    // Top 2 should be src1, src3
    BOOST_CHECK(recorder.data().size() == 2);
    top1 = std::min(recorder.data()[0].first, recorder.data()[1].first);
    top2 = std::max(recorder.data()[0].first, recorder.data()[1].first);
    BOOST_CHECK(top1 == "src1");
    BOOST_CHECK(top2 == "src3");
}

BOOST_AUTO_TEST_CASE(DetectMute)
{
    owt_base::AudioRanker ranker(&recorder, false, 10);
    ranker.addOutput(&dest1);
    ranker.addInput(&src1, "src1", "owner1");
    ranker.addInput(&src2, "src2", "owner2");

    // Output dest1 should be linked with first input src1
    frame.additionalInfo.audio.audioLevel = 50;
    src1.generateFrame(frame);
    frame.additionalInfo.audio.audioLevel = 60;
    src2.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
    BOOST_CHECK(recorder.data().front().first == "src1");

    // Since no frame detect time is 200ms
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    frame.additionalInfo.audio.audioLevel = 60;
    src2.generateFrame(frame);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    frame.additionalInfo.audio.audioLevel = 60;
    src2.generateFrame(frame);

    boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
    BOOST_CHECK(recorder.data().front().first == "src2");
}

BOOST_AUTO_TEST_SUITE_END()
