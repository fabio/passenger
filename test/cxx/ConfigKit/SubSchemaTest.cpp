#include <TestSupport.h>
#include <ConfigKit/ConfigKit.h>

using namespace Passenger;
using namespace std;

namespace tut {
	struct ConfigKit_SubSchemaTest {
		ConfigKit::Schema schema;
		ConfigKit::Schema subschema;
		ConfigKit::TableTranslator translator;
		Json::Value doc;
		vector<ConfigKit::Error> errors;
	};

	DEFINE_TEST_GROUP(ConfigKit_SubSchemaTest);

	TEST_METHOD(1) {
		set_test_name("The subschema's entries are added to the parent schema's entries");

		subschema.add("gender", ConfigKit::STRING_TYPE, ConfigKit::OPTIONAL);
		subschema.finalize();

		schema.add("name", ConfigKit::STRING_TYPE, ConfigKit::OPTIONAL);
		schema.addSubSchema(subschema);
		schema.finalize();

		Json::Value desc = schema.inspect();
		ensure(desc.isMember("name"));
		ensure(desc.isMember("gender"));
	}

	TEST_METHOD(2) {
		set_test_name("When adding subschema entries,"
			" they are translated to the parent schema's equivalent names");

		subschema.add("gender", ConfigKit::STRING_TYPE, ConfigKit::REQUIRED);
		subschema.finalize();

		translator.add("sub_gender", "gender");
		schema.addSubSchema(subschema, translator);
		schema.finalize();

		Json::Value desc = schema.inspect();
		ensure(desc.isMember("sub_gender"));
		ensure(!desc.isMember("gender"));
	}

	TEST_METHOD(3) {
		set_test_name("The subschema's type validators are compatible with translations");

		subschema.add("gender", ConfigKit::STRING_TYPE, ConfigKit::REQUIRED);
		subschema.finalize();

		translator.add("sub_gender", "gender");
		schema.addSubSchema(subschema, translator);
		schema.finalize();

		ConfigKit::Store config(schema);
		config.previewUpdate(doc, errors);
		ensure_equals(errors.size(), 1u);
		ensure_equals(errors[0].getMessage(), "'sub_gender' is required");
	}

	static Json::Value inferDefaultValueForGender(const ConfigKit::Store *config) {
		return config->get("default_gender");
	}

	TEST_METHOD(4) {
		set_test_name("The subschema's dynamic default values are compatible with translations");

		subschema.add("default_gender", ConfigKit::STRING_TYPE, ConfigKit::OPTIONAL, "male");
		subschema.addWithDynamicDefault("gender", ConfigKit::STRING_TYPE,
			ConfigKit::OPTIONAL, inferDefaultValueForGender);
		subschema.finalize();

		translator.add("sub_default_gender", "default_gender");
		translator.add("sub_gender", "gender");
		translator.finalize();
		schema.addSubSchema(subschema, translator);
		schema.finalize();

		ConfigKit::Store config(schema);
		ensure_equals(config["sub_gender"].asString(), "male");
	}

	static void validateSubschema(const ConfigKit::Store &config,
		vector<ConfigKit::Error> &errors)
	{
		if (config["gender"].asString() != "male" && config["gender"].asString() != "female") {
			errors.push_back(ConfigKit::Error("{{gender}} must be male or female"));
		}
	}

	TEST_METHOD(10) {
		set_test_name("The subschema's custom validators work on the main schema too");

		subschema.add("gender", ConfigKit::STRING_TYPE, ConfigKit::REQUIRED);
		subschema.addValidator(validateSubschema);
		subschema.finalize();

		schema.addSubSchema(subschema);
		schema.finalize();

		ConfigKit::Store config(schema);
		doc["gender"] = "none";
		config.previewUpdate(doc, errors);
		ensure_equals(errors.size(), 1u);
		ensure_equals(errors[0].getMessage(), "gender must be male or female");
	}

	TEST_METHOD(11) {
		set_test_name("The subschema's custom validators are compatible with translations");

		subschema.add("gender", ConfigKit::STRING_TYPE, ConfigKit::REQUIRED);
		subschema.addValidator(validateSubschema);
		subschema.finalize();

		translator.add("sub_gender", "gender");
		translator.finalize();
		schema.addSubSchema(subschema, translator);
		schema.finalize();

		ConfigKit::Store config(schema);
		doc["sub_gender"] = "none";
		config.previewUpdate(doc, errors);
		ensure_equals(errors.size(), 1u);
		ensure_equals(errors[0].getMessage(), "sub_gender must be male or female");
	}
}
