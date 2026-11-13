#include "FactValueSliderListModelTest.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <memory>

#include "Fact.h"
#include "FactMetaData.h"
#include "FactValueSliderListModel.h"

static Fact *_createDoubleFact(double value, double min, double max, double increment, int decimals,
                               QObject *parent)
{
    Fact *fact = new Fact(0, "slider", FactMetaData::valueTypeDouble, parent);
    FactMetaData *meta = fact->metaData();
    meta->setRawMin(QVariant(min));
    meta->setRawMax(QVariant(max));
    meta->setDecimalPlaces(decimals);
    meta->setRawIncrement(increment);
    fact->setRawValue(QVariant(value));
    return fact;
}

// Access private rowCount/roleNames through the public QAbstractListModel interface
static int _rowCount(QAbstractListModel &model)
{
    return model.rowCount();
}

static QHash<int, QByteArray> _roleNames(QAbstractListModel &model)
{
    return model.roleNames();
}

void FactValueSliderListModelTest::_resetInitialValueBasic_test()
{
    Fact *fact = _createDoubleFact(50.0, 0.0, 100.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    const int initialIndex = model.resetInitialValue();

    // prevValues = min((50-0)/1, 100) = 50
    // nextValues = min((100-50)/1, 100) = 50
    // total = 50 + 1 + 50 = 101
    QCOMPARE(_rowCount(model), 101);
    QCOMPARE(initialIndex, 50);
}

void FactValueSliderListModelTest::_rowCountMatchesTotalValues_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();

    // prevValues = 5, nextValues = 5, total = 11
    QCOMPARE(_rowCount(model), 11);
}

void FactValueSliderListModelTest::_valueAtInitialIndex_test()
{
    Fact *fact = _createDoubleFact(50.0, 0.0, 100.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    const int initialIndex = model.resetInitialValue();
    const double valueAtInitial = model.valueAtModelIndex(initialIndex);

    QCOMPARE_FUZZY(valueAtInitial, 50.0, 0.01);
    QCOMPARE_FUZZY(model.initialValueAtPrecision(), 50.0, 0.01);
}

void FactValueSliderListModelTest::_valueAtModelIndexBoundaries_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();

    // First index (index 0) should be near min
    const double first = model.valueAtModelIndex(0);
    QCOMPARE_FUZZY(first, 0.0, 0.01);

    // Last index should be near max
    const double last = model.valueAtModelIndex(_rowCount(model) - 1);
    QCOMPARE_FUZZY(last, 10.0, 0.01);
}

void FactValueSliderListModelTest::_valueIndexAtModelIndex_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();

    QCOMPARE(model.valueIndexAtModelIndex(0), 0);
    QCOMPARE(model.valueIndexAtModelIndex(5), 5);
    QCOMPARE(model.valueIndexAtModelIndex(10), 10);
}

void FactValueSliderListModelTest::_roleNames_test()
{
    Fact *fact = _createDoubleFact(0.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    const QHash<int, QByteArray> roles = _roleNames(model);
    QVERIFY(roles.values().contains("value"));
    QVERIFY(roles.values().contains("valueIndex"));
}

void FactValueSliderListModelTest::_resetClearsOldRows_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();
    const int firstCount = _rowCount(model);
    QVERIFY(firstCount > 0);

    // Change value and reset — should get fresh row count
    fact->setRawValue(QVariant(2.0));
    model.resetInitialValue();

    // prevValues = 2, nextValues = 8, total = 11
    QCOMPARE(_rowCount(model), 11);
}

void FactValueSliderListModelTest::_integerIncrement_test()
{
    Fact *fact = _createDoubleFact(50.0, 0.0, 100.0, 5.0, 0, this);
    FactValueSliderListModel model(*fact);

    const int initialIndex = model.resetInitialValue();

    // prevValues = min((50-0)/5, 100) = 10
    // nextValues = min((100-50)/5, 100) = 10
    // total = 10 + 1 + 10 = 21
    QCOMPARE(_rowCount(model), 21);

    // Values should step by 5
    const double v0 = model.valueAtModelIndex(0);
    const double v1 = model.valueAtModelIndex(1);
    QCOMPARE_FUZZY(v1 - v0, 5.0, 0.01);

    QCOMPARE_FUZZY(model.valueAtModelIndex(initialIndex), 50.0, 0.01);
}

/// The slider model dereferences its Fact and the Fact caches the pointer, so the model must
/// live and die with that Fact. A parentless model that QML reads would be adopted by the
/// JavaScript engine and deleted when its wrapper is collected, leaving the Fact's cached
/// pointer dangling; with no owner at all it would simply leak.
void FactValueSliderListModelTest::_modelIsOwnedAndDestroyedWithFact_test()
{
    Fact *const fact = new Fact(0, QStringLiteral("slider"), FactMetaData::valueTypeDouble);
    FactValueSliderListModel *const model = fact->valueSliderModel();
    QVERIFY(model != nullptr);

    // One shared instance per Fact: every QML consumer borrows the same model.
    QCOMPARE(fact->valueSliderModel(), model);
    QCOMPARE(model->parent(), fact);
    QCOMPARE(QQmlEngine::objectOwnership(model), QQmlEngine::CppOwnership);

    QPointer<FactValueSliderListModel> guard(model);
    delete fact;
    QVERIFY2(guard.isNull(), "the slider model must be destroyed together with its Fact");
}

/// Reading the model from QML must not hand ownership to the engine.
void FactValueSliderListModelTest::_modelStaysCppOwnedWhenExposedToQml_test()
{
    Fact *const fact = new Fact(0, QStringLiteral("slider"), FactMetaData::valueTypeDouble);

    {
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("testFact"), fact);

        QQmlComponent component(&engine);
        component.setData(QByteArrayLiteral("import QtQml\n"
                                            "QtObject { property var model: testFact.valueSliderModel() }"),
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object != nullptr, qPrintable(component.errorString()));
        QCoreApplication::processEvents();

        QCOMPARE(QQmlEngine::objectOwnership(fact->valueSliderModel()), QQmlEngine::CppOwnership);

        object.reset();
        engine.collectGarbage();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // The engine is gone and the model is still alive and reachable through the Fact.
    QVERIFY(fact->valueSliderModel() != nullptr);
    delete fact;
}

UT_REGISTER_TEST(FactValueSliderListModelTest, TestLabel::Unit)
