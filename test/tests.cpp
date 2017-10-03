
#include <vtzero/builder.hpp>
#include <vtzero/geometry.hpp>
#include <vtzero/index.hpp>
#include <vtzero/property_value.hpp>
#include <vtzero/property_value_view.hpp>

#include <catch.hpp>

#ifdef VTZERO_TEST_WITH_VARIANT
# include <boost/variant.hpp>
#endif

#include <map>
#include <unordered_map>

struct visitor_test_void {

    int x = 0;

    template <typename T>
    void operator()(T /*value*/) {
        x = 1;
    }

    void operator()(vtzero::data_view /*value*/) {
        x = 2;
    }

};

struct visitor_test_int {

    template <typename T>
    int operator()(T /*value*/) {
        return 1;
    }

    int operator()(vtzero::data_view /*value*/) {
        return 2;
    }

};

struct visitor_test_to_string {

    template <typename T>
    std::string operator()(T value) {
        return std::to_string(value);
    }

    std::string operator()(vtzero::data_view value) {
        return std::string{value.data(), value.size()};
    }

};

#ifdef VTZERO_TEST_WITH_VARIANT
using variant_type = boost::variant<std::string, float, double, int64_t, uint64_t, bool>;
#endif

TEST_CASE("string value") {
    vtzero::property_value v{"foo"};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.string_value() == "foo");

    visitor_test_void vt;
    vtzero::apply_visitor(vt, vv);
    REQUIRE(vt.x == 2);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, vv);
    REQUIRE(result == 2);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "foo");

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type>(vv);
    REQUIRE(boost::get<std::string>(vari) == "foo");
#endif
}

TEST_CASE("float value") {
    vtzero::property_value v{1.2f};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.float_value() == Approx(1.2));

    visitor_test_void vt;
    vtzero::apply_visitor(vt, vv);
    REQUIRE(vt.x == 1);

    const auto result = vtzero::apply_visitor(visitor_test_int{}, vv);
    REQUIRE(result == 1);

#ifdef VTZERO_TEST_WITH_VARIANT
    const auto vari = vtzero::convert_property_value<variant_type, std::string>(vv);
    REQUIRE(boost::get<float>(vari) == Approx(1.2));
#endif
}

TEST_CASE("double value") {
    vtzero::property_value v{1.2};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.double_value() == Approx(1.2));
}

TEST_CASE("int value") {
    vtzero::property_value v{vtzero::int_value_type{42}};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.int_value() == 42);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "42");
}

TEST_CASE("uint value") {
    vtzero::property_value v{vtzero::uint_value_type{99}};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.uint_value() == 99);

    const auto str = vtzero::apply_visitor(visitor_test_to_string{}, vv);
    REQUIRE(str == "99");
}

TEST_CASE("sint value") {
    vtzero::property_value v{vtzero::sint_value_type{42}};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.sint_value() == 42);
}

TEST_CASE("bool value") {
    vtzero::property_value v{true};
    vtzero::property_value_view vv{v.data()};
    REQUIRE(vv.bool_value());
}

TEST_CASE("property map") {
    vtzero::tile_builder tile;
    vtzero::layer_builder layer_points{tile, "points"};
    {
        vtzero::point_feature_builder feature{layer_points, 1};
        feature.add_points(1);
        feature.set_point(10, 10);
        feature.add_property("foo", "bar");
        feature.add_property("x", "y");
        feature.add_property("abc", "def");
    }

    std::string data = tile.serialize();

    vtzero::vector_tile vt{data};
    REQUIRE(vt.size() == 1);
    auto layer = *vt.begin();
    REQUIRE(layer.size() == 1);
    auto feature = *layer.begin();

    REQUIRE(feature.size() == 3);

#ifdef VTZERO_TEST_WITH_VARIANT
    SECTION("std::map") {
        using prop_map_type = std::map<std::string, variant_type>;
        auto map = vtzero::create_properties_map<prop_map_type>(feature);

        REQUIRE(map.size() == 3);
        REQUIRE(boost::get<std::string>(map["foo"]) == "bar");
        REQUIRE(boost::get<std::string>(map["x"]) == "y");
        REQUIRE(boost::get<std::string>(map["abc"]) == "def");
    }
    SECTION("std::unordered_map") {
        using prop_map_type = std::unordered_map<std::string, variant_type>;
        auto map = vtzero::create_properties_map<prop_map_type>(feature);

        REQUIRE(map.size() == 3);
        REQUIRE(boost::get<std::string>(map["foo"]) == "bar");
        REQUIRE(boost::get<std::string>(map["x"]) == "y");
        REQUIRE(boost::get<std::string>(map["abc"]) == "def");
    }
#endif
}

struct point_handler {

    std::vector<vtzero::point> data;

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point point) {
        data.push_back(point);
    }

    void points_end() const noexcept {
    }

};

struct mypoint {
    int64_t p1;
    int64_t p2;
};

/*inline vtzero::point create_point(mypoint p) noexcept {
    return {static_cast<int32_t>(p.p1),
            static_cast<int32_t>(p.p2)};
}*/

namespace vtzero {

    template <>
    point create_point<::mypoint>(::mypoint p) noexcept {
        return {static_cast<int32_t>(p.p1),
                static_cast<int32_t>(p.p2)};
    }
}

TEST_CASE("Point builder") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};

    vtzero::point_feature_builder fbuilder{lbuilder, 17};

    SECTION("add point using coordinates") {
        fbuilder.add_point(10, 20);
    }

    SECTION("add point using vtzero::point") {
        fbuilder.add_point(vtzero::point{10, 20});
    }

    SECTION("add point using mypoint") {
        fbuilder.add_point(mypoint{10, 20});
    }

    fbuilder.commit();

    std::string data = tbuilder.serialize();

    vtzero::vector_tile tile{data};

    const auto layer = *tile.begin();
    REQUIRE(layer.name() == "test");
    REQUIRE(layer.version() == 2);
    REQUIRE(layer.extent() == 4096);
    REQUIRE(layer.size() == 1);

    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 17);

    point_handler handler;
    vtzero::decode_point_geometry(feature.geometry(), true, handler);

    std::vector<vtzero::point> result = {{10, 20}};
    REQUIRE(handler.data == result);
}

TEST_CASE("value index") {
    vtzero::tile_builder tbuilder;
    vtzero::layer_builder lbuilder{tbuilder, "test"};
    const auto key = lbuilder.add_key("some_key");

    vtzero::point_feature_builder fbuilder{lbuilder, 17};
    fbuilder.add_point(10, 20);

    SECTION("no index") {
        fbuilder.add_property(key, vtzero::sint_value_type{12});
    }

    SECTION("external value index using unordered_map") {
        vtzero::value_index<vtzero::sint_value_type, int, std::unordered_map> index{lbuilder};
        fbuilder.add_property(key, index(12));
    }

    SECTION("external value index using map") {
        vtzero::value_index<vtzero::sint_value_type, int, std::map> index{lbuilder};
        fbuilder.add_property(key, index(12));
    }

    SECTION("property_value_type index") {
        vtzero::value_index_internal<std::unordered_map> index{lbuilder};
        fbuilder.add_property(key, index(vtzero::property_value{vtzero::sint_value_type{12}}));
    }

    fbuilder.commit();

    std::string data = tbuilder.serialize();

    // ============

    vtzero::vector_tile tile{data};

    REQUIRE(tile.size() == 1);
    const auto layer = *tile.begin();
    REQUIRE(layer.size() == 1);
    const auto feature = *layer.begin();
    REQUIRE(feature.id() == 17);
    const auto property = *feature.begin();
    REQUIRE(property.value().sint_value() == 12);
}

