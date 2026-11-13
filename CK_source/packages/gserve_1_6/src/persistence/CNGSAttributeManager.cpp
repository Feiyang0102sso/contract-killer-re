// CNGSAttributeManager.cpp - CAttributeManager: avatar attribute requests and reply parsing (b025x).
#include "CNGSAttributeManager.h"
#include "CNGSAttribute.h"
#include "CNGSTokens.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSServerRequest.h"
#include "CNGSUtil.h"
#include "CObjectMap.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"
#include "CStrChar.h"


CObjectMapObject* CAttributeManager::createAttributeMessageObject(const TCVector<CNGSAttribute*>& attributes)
{
    CObjectMapArray* list = new CObjectMapArray();
    for (int index = 0; index < attributes.Size(); index++) {
        // The attribute's JSON object is its first member.
        list->addEntry((CObjectMapValue*)attributes[index]);
    }
    CObjectMapObject* message = new CObjectMapObject();
    message->addEntry(CStrWChar(NGS_AVATAR_ATTRIBUTES_TOKEN), list);
    return message;
}

unsigned int CAttributeManager::saveDataToServer(const TCVector<CNGSAttribute*>& attributes, CNGSServerRequestFunctor* functor,
                                                 const CNGSUserCredentials* credentials, unsigned char noSession)
{
    CObjectMapObject* message = createAttributeMessageObject(attributes);
    CNGS::GetInstance()->GetLocalUser()->InsertHeaderIntoMessageObject(message, NGS_MESSAGE_TYPE_AVATAR_UPDATE_ITEM_STATS, noSession);
    CObjectMapObject* user = credentials->GetUserObject();
    CObjectMapObject* avatar = credentials->GetAvatarObject();
    message->addEntry(CStrWChar(NGS_USER_TOKEN), user);
    message->addEntry(CStrWChar(NGS_REGISTER_AVATAR_TOKEN), avatar);

    CObjectMap* map = new CObjectMap(message);
    delete message;

    CStrChar url(CNGS::GetInstance()->GetServerURL());
    url += "resources/avatar/avatarattributes";
    CNGSServerRequest* request = new CNGSServerRequest(url.GetCharPtr(), map, functor, 0, 1);
    return request->m_messageId;
}

unsigned int CAttributeManager::getDataFromServer(const TCVector<int>& collectionTypes, CNGSServerRequestFunctor* functor,
                                                  const CNGSUserCredentials* credentials)
{
    TCVector<unsigned int> types;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        unsigned int type = collectionTypes[index];
        types.Add(type);
    }

    int count = types.Size();
    const unsigned int* collectionIds = types.m_data;
    CObjectMapObject* message = new CObjectMapObject();
    CObjectMapArray* list = new CObjectMapArray();
    for (int index = 0; index < count; index++) {
        CObjectMapObject* entry = new CObjectMapObject();
        entry->addEntry(CStrWChar(NGS_COLLECTION_TOKEN), new CObjectMapInt(collectionIds[index]));
        list->addEntry(entry);
    }

    CObjectMapObject* user = credentials->GetUserObject();
    CObjectMapObject* avatar = credentials->GetAvatarObject();
    message->addEntry(CStrWChar(NGS_USER_TOKEN), user);
    if (avatar != NULL) {
        message->addEntry(CStrWChar(NGS_REGISTER_AVATAR_TOKEN), avatar);
    }
    message->addEntry(CStrWChar(NGS_AVATAR_ATTRIBUTES_TOKEN), list);
    CNGS::GetInstance()->GetLocalUser()->InsertHeaderIntoMessageObject(message, NGS_MESSAGE_TYPE_AVATAR_QUERY_ATTRIBUTES, 0);

    CObjectMap* map = new CObjectMap(message);
    delete message;

    CStrChar url(CNGS::GetInstance()->GetServerURL());
    url += "resources/avatar/queryattributes";
    CNGSServerRequest* request = new CNGSServerRequest(url.GetCharPtr(), map, functor, 0, 1);
    return request->m_messageId;
}

void CAttributeManager::extractAttributeValuesAsArray(CObjectMap* response, CNGSServerRequestFunctor* functor,
                                                      TCVector<CNGSAttribute*>& attributes)
{
    if (CNGSServerObject::WasErrorInResponse(response, functor, "HandleAvatarQueryAttributesResponse") == 0) {
        CObjectMapValue* list = response->getDataAt(CStrWChar(NGS_LIST_TOKEN), CStrWChar(L" "));
        if (list != NULL && list->getType() == CObjectMapValue::TYPE_ARRAY) {
            CObjectMapObject* root = (CObjectMapObject*)response->getDataAt(CStrWChar(NGS_LIST_TOKEN), CStrWChar(L" "));
            extractAttributeValuesAsArray(root, attributes, true);
        }
    }
}

unsigned char CAttributeManager::extractAttributeValuesAsArray(const CObjectMapObject* root, TCVector<CNGSAttribute*>& attributes,
                                                               bool isList)
{
    // The array is read through the CObjectMapObject layout (its values vector sits at the same offset).
    const CObjectMapObject* list = root;
    if (!isList) {
        list = (const CObjectMapObject*)root->getEntry(CStrWChar(L"avatarattributes"));
    }
    unsigned char success = 0;
    if (list != NULL) {
        success = 1;
        for (int index = 0; index < list->m_values.Size(); index++) {
            CObjectMapObject* entry = (CObjectMapObject*)list->m_values[index];
            if (entry != NULL && entry->getType() == CObjectMapValue::TYPE_OBJECT) {
                CObjectMapString* nameValue = (CObjectMapString*)entry->getEntry(CStrWChar(L"name"));
                CStrWChar name(nameValue->m_value.GetWCharPtr());
                CObjectMapInt* typeValue = (CObjectMapInt*)entry->getEntry(CStrWChar(L"type"));
                int type = (int)typeValue->m_value;
                CObjectMapInt* collectionTypeValue = (CObjectMapInt*)entry->getEntry(CStrWChar(L"collectiontype"));
                int collectionType = (int)collectionTypeValue->m_value;
                CNGSAttribute* attribute = NULL;
                if (type == DATA_TYPE_STRING || type == DATA_TYPE_BASE64) {
                    CObjectMapString* value = (CObjectMapString*)entry->getEntry(CStrWChar(L"stringValue"));
                    CStrWChar text(value->m_value.GetWCharPtr());
                    attribute = new CNGSAttribute(name, text, type == DATA_TYPE_BASE64, collectionType);
                } else {
                    CObjectMapInt* value = (CObjectMapInt*)entry->getEntry(CStrWChar(L"intValue"));
                    if (type >= DATA_TYPE_UINT8 && type <= DATA_TYPE_INT64) {
                        // Read the complete numeric value before allocating its attribute.
                        long long number = value->m_value;
                        switch (type) {
                        case DATA_TYPE_UINT8:
                            attribute = new CNGSAttribute(name, (unsigned char)number, collectionType);
                            break;
                        case DATA_TYPE_UINT16:
                            attribute = new CNGSAttribute(name, (unsigned short)number, collectionType);
                            break;
                        case DATA_TYPE_UINT32:
                            attribute = new CNGSAttribute(name, (unsigned int)number, collectionType);
                            break;
                        case DATA_TYPE_UINT64:
                            attribute = new CNGSAttribute(name, (unsigned long long)number, collectionType);
                            break;
                        case DATA_TYPE_INT8:
                            attribute = new CNGSAttribute(name, (signed char)number, collectionType);
                            break;
                        case DATA_TYPE_INT16:
                            attribute = new CNGSAttribute(name, (short)number, collectionType);
                            break;
                        case DATA_TYPE_INT32:
                            attribute = new CNGSAttribute(name, (int)number, collectionType);
                            break;
                        case DATA_TYPE_INT64:
                            attribute = new CNGSAttribute(name, number, collectionType);
                            break;
                        }
                    } else {
                        success = 0;
                    }
                }
                if (attribute != NULL) {
                    // Nameless attributes are dropped (and leaked, as in the original).
                    int nameLength = attribute->getName().GetLength();
                    if (nameLength > 0) {
                        attributes.Add(attribute);
                    }
                    attribute->debugPrint();
                }
            } else {
                success = 0;
            }
        }
    }
    return success;
}

int CAttributeManager::extractAttributeValuesAsHashMap(const CObjectMapObject* root, CHash& attributes)
{
    TCVector<CNGSAttribute*> list;
    extractAttributeValuesAsArray(root, list, false);
    for (int index = 0; index < list.Size(); index++) {
        CNGSAttribute* attribute = list[index];
        CStrChar name = CNGSUtil::WStrToCStr(CStrWChar(attribute->getName().GetWCharPtr()));
        attributes.Insert(CStringToKey(name.GetCharPtr(), 0), attribute);
    }
    return list.Size();
}

int CAttributeManager::extractAttributeValuesAsHashMap(CObjectMap* response, CNGSServerRequestFunctor* functor,
                                                       CHash& attributes)
{
    TCVector<CNGSAttribute*> list;
    extractAttributeValuesAsArray(response, functor, list);
    for (int index = 0; index < list.Size(); index++) {
        CNGSAttribute* attribute = list[index];
        CStrChar name = CNGSUtil::WStrToCStr(CStrWChar(attribute->getName().GetWCharPtr()));
        attributes.Insert(CStringToKey(name.GetCharPtr(), 0), attribute);
    }
    return list.Size();
}
