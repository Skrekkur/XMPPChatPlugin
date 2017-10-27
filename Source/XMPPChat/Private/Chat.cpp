// (c) 2015 Descendent Studios, Inc.

#include "XMPPChatPrivatePCH.h"
#include "Chat.h"

#include "ModuleManager.h"
#include "Xmpp.h"
#include "XmppConnection.h"

DEFINE_LOG_CATEGORY(LogChat);

UChatMember::UChatMember(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	Status(EUXmppPresenceStatus::Offline),
	bIsAvailable(false),
	Role(EUChatMemberRole::Participant)
{
}

void UChatMember::ConvertFrom(const FXmppChatMember& ChatMember)
{
	Nickname = ChatMember.Nickname;
	MemberJid = ChatMember.MemberJid.GetFullPath();
	Status = UChatUtil::GetEUXmppPresenceStatus(ChatMember.UserPresence.Status);
	bIsAvailable = ChatMember.UserPresence.bIsAvailable;
	SentTime = ChatMember.UserPresence.SentTime;
	//ClientResource = ChatMember.UserPresence.ClientResource;
	//NickName = ChatMember.UserPresence.NickName;
	StatusStr = ChatMember.UserPresence.StatusStr;
	Role = UChatUtil::GetEUChatMemberRole(ChatMember.Role);
}

/***************** Base **************************/

UChat::UChat(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), bInited(false), bDone(false)
{
}

UChat::~UChat()
{
	DeInit();
}

void UChat::Init()
{
	if (XmppConnection.IsValid() && !bInited)
	{
		bInited = true;

		IXmppConnection::FOnXmppLoginComplete& OnXMPPLoginCompleteDelegate = XmppConnection->OnLoginComplete();
		OnLoginCompleteHandle = OnXMPPLoginCompleteDelegate.AddUObject(this, &UChat::OnLoginCompleteFunc);

		IXmppConnection::FOnXmppLogoutComplete& OnXMPPLogoutCompleteDelegate = XmppConnection->OnLogoutComplete();
		OnLogoutCompleteHandle = OnXMPPLogoutCompleteDelegate.AddUObject(this, &UChat::OnLogoutCompleteFunc);

		IXmppConnection::FOnXmppLogingChanged& OnXMPPLogingChangedDelegate = XmppConnection->OnLoginChanged();
		OnLogingChangedHandle = OnXMPPLogingChangedDelegate.AddUObject(this, &UChat::OnLogingChangedFunc);

		if (XmppConnection->Messages().IsValid())
		{
			IXmppMessages::FOnXmppMessageReceived& OnXMPPReceiveMessageDelegate = XmppConnection->Messages()->OnReceiveMessage();
			OnChatReceiveMessageHandle = OnXMPPReceiveMessageDelegate.AddUObject(this, &UChat::OnChatReceiveMessageFunc);
		}

		if (XmppConnection->PrivateChat().IsValid())
		{
			IXmppChat::FOnXmppChatReceived& OnXMPPChatReceivedDelegate = XmppConnection->PrivateChat()->OnReceiveChat();
			OnPrivateChatReceiveMessageHandle = OnXMPPChatReceivedDelegate.AddUObject(this, &UChat::OnPrivateChatReceiveMessageFunc);
		}

		if (XmppConnection->MultiUserChat().IsValid())
		{
			IXmppMultiUserChat::FOnXmppRoomChatReceived& OnXMPPMUCReceiveMessageDelegate = XmppConnection->MultiUserChat()->OnRoomChatReceived();
			OnMUCReceiveMessageHandle = OnXMPPMUCReceiveMessageDelegate.AddUObject(this, &UChat::OnMUCReceiveMessageFunc);

			IXmppMultiUserChat::FOnXmppRoomJoinPublicComplete& OnXMPPMUCRoomJoinPublicDelegate = XmppConnection->MultiUserChat()->OnJoinPublicRoom();
			OnMUCRoomJoinPublicCompleteHandle = OnXMPPMUCRoomJoinPublicDelegate.AddUObject(this, &UChat::OnMUCRoomJoinPublicCompleteFunc);

			IXmppMultiUserChat::FOnXmppRoomJoinPrivateComplete& OnXMPPMUCRoomJoinPrivateDelegate = XmppConnection->MultiUserChat()->OnJoinPrivateRoom();
			OnMUCRoomJoinPrivateCompleteHandle = OnXMPPMUCRoomJoinPrivateDelegate.AddUObject(this, &UChat::OnMUCRoomJoinPrivateCompleteFunc);

			IXmppMultiUserChat::FOnXmppRoomMemberJoin& OnXMPPRoomMemberJoinDelegate = XmppConnection->MultiUserChat()->OnRoomMemberJoin();
			OnMUCRoomMemberJoinHandle = OnXMPPRoomMemberJoinDelegate.AddUObject(this, &UChat::OnMUCRoomMemberJoinFunc);

			IXmppMultiUserChat::FOnXmppRoomMemberExit& OnXMPPRoomMemberExitDelegate = XmppConnection->MultiUserChat()->OnRoomMemberExit();
			OnMUCRoomMemberExitHandle = OnXMPPRoomMemberExitDelegate.AddUObject(this, &UChat::OnMUCRoomMemberExitFunc);

			IXmppMultiUserChat::FOnXmppRoomMemberChanged& OnXMPPRoomMemberChangedDelegate = XmppConnection->MultiUserChat()->OnRoomMemberChanged();
			OnMUCRoomMemberChangedHandle = OnXMPPRoomMemberChangedDelegate.AddUObject(this, &UChat::OnMUCRoomMemberChangedFunc);
		}
	}
}

void UChat::DeInit()
{
	if (XmppConnection.IsValid())
	{
		bInited = false;

		if (OnLoginCompleteHandle.IsValid()) { XmppConnection->OnLoginComplete().Remove(OnLoginCompleteHandle); }
		if (OnLogoutCompleteHandle.IsValid()) { XmppConnection->OnLogoutComplete().Remove(OnLogoutCompleteHandle); }
		if (OnLogingChangedHandle.IsValid()) { XmppConnection->OnLoginChanged().Remove(OnLogingChangedHandle); }
		if (OnChatReceiveMessageHandle.IsValid()) { XmppConnection->Messages()->OnReceiveMessage().Remove(OnChatReceiveMessageHandle); }
		if (OnPrivateChatReceiveMessageHandle.IsValid()) { XmppConnection->PrivateChat()->OnReceiveChat().Remove(OnPrivateChatReceiveMessageHandle); }
		if (OnMUCReceiveMessageHandle.IsValid()) { XmppConnection->MultiUserChat()->OnRoomChatReceived().Remove(OnMUCReceiveMessageHandle); }
		if (OnMUCRoomJoinPublicCompleteHandle.IsValid()) { XmppConnection->MultiUserChat()->OnJoinPublicRoom().Remove(OnMUCRoomJoinPublicCompleteHandle); }
		if (OnMUCRoomJoinPrivateCompleteHandle.IsValid()) { XmppConnection->MultiUserChat()->OnJoinPrivateRoom().Remove(OnMUCRoomJoinPrivateCompleteHandle); }
		if (OnMUCRoomMemberJoinHandle.IsValid()) { XmppConnection->MultiUserChat()->OnRoomMemberJoin().Remove(OnMUCRoomMemberJoinHandle); }
		if (OnMUCRoomMemberExitHandle.IsValid()) { XmppConnection->MultiUserChat()->OnRoomMemberExit().Remove(OnMUCRoomMemberExitHandle); }
		if (OnMUCRoomMemberChangedHandle.IsValid()) { XmppConnection->MultiUserChat()->OnRoomMemberChanged().Remove(OnMUCRoomMemberChangedHandle); }

		FXmppModule::Get().RemoveConnection(XmppConnection.ToSharedRef());
	}	
}

void UChat::Finish()
{
	if (XmppConnection.IsValid())
	{
		if (XmppConnection->GetLoginStatus() == EXmppLoginStatus::LoggedIn)
		{
			Logout();
		}
		else
		{
			bDone = true;
		}
	}
}

/***************** Login/Logout **************************/

void UChat::Login(const FString& UserId, const FString& Auth, const FString& ServerAddr, const FString& Domain, const FString& ClientResource)
{
	FXmppServer XmppServer;
	XmppServer.ServerAddr = ServerAddr;
	XmppServer.Domain = Domain;
	XmppServer.ClientResource = ClientResource;

	Login(UserId, Auth, XmppServer);
}

void UChat::Login(const FString& UserId, const FString& Auth, const FXmppServer& XmppServer)
{
	FXmppModule& Module = FModuleManager::GetModuleChecked<FXmppModule>("XMPP");

	UE_LOG(LogChat, Log, TEXT("UChat::Login enabled=%s UserId=%s"), (Module.IsXmppEnabled() ? TEXT("true") : TEXT("false")), *UserId );

	XmppConnection = Module.CreateConnection(UserId);

	if (XmppConnection.IsValid())
	{
		//UE_LOG(LogChat, Log, TEXT("UChat::Login XmppConnection is %s"), typeid(*XmppConnection).name() );
		UE_LOG(LogChat, Log, TEXT("UChat::Login XmppConnection valid") );

		Init();

		XmppConnection->SetServer(XmppServer);

		XmppConnection->Login(UserId, Auth);
	}
	else
	{
		UE_LOG(LogChat, Error, TEXT("UChat::Login XmppConnection not valid, failed.  UserId=%s"), *UserId );
	}
}

void UChat::OnLoginCompleteFunc(const FXmppUserJid& UserJid, bool bWasSuccess, const FString& Error)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnLoginComplete UserJid=%s Success=%s Error=%s"),	*UserJid.GetFullPath(), bWasSuccess ? TEXT("true") : TEXT("false"), *Error);

	OnChatLoginComplete.Broadcast(UserJid.GetFullPath(), bWasSuccess, Error);
}

void UChat::OnLogoutCompleteFunc(const FXmppUserJid& UserJid, bool bWasSuccess, const FString& Error)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnLogoutComplete UserJid=%s Success=%s Error=%s"), *UserJid.GetFullPath(), bWasSuccess ? TEXT("true") : TEXT("false"), *Error);	

	OnChatLogoutComplete.Broadcast(UserJid.GetFullPath(), bWasSuccess, Error);
}

void UChat::OnLogingChangedFunc(const FXmppUserJid& UserJid, EXmppLoginStatus::Type LoginStatus)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnLogingChanged UserJid=%s LoginStatus=%d"), *UserJid.GetFullPath(), static_cast<int32>(LoginStatus));

	OnChatLogingChanged.Broadcast(UserJid.GetFullPath(), UChatUtil::GetEUXmppLoginStatus(LoginStatus));
}

void UChat::Logout()
{
	if (XmppConnection.IsValid() && (XmppConnection->GetLoginStatus() == EXmppLoginStatus::LoggedIn))
	{
		XmppConnection->Logout();
	}
}

/***************** Chat **************************/

void UChat::OnChatReceiveMessageFunc(const TSharedRef<IXmppConnection>& Connection, const FXmppUserJid& FromJid, const TSharedRef<FXmppMessage>& Message)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnChatReceiveMessage UserJid=%s Type=%s Message=%s"), *FromJid.GetFullPath(), *Message->Type, *Message->Payload);

	OnChatReceiveMessage.Broadcast(FromJid.GetFullPath(), Message->Type, Message->Payload);
}

void UChat::OnPrivateChatReceiveMessageFunc(const TSharedRef<IXmppConnection>& Connection, const FXmppUserJid& FromJid, const TSharedRef<FXmppChatMessage>& Message)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnPrivateChatReceiveMessage UserJid=%s Message=%s"), *FromJid.GetFullPath(), *Message->Body);

	OnPrivateChatReceiveMessage.Broadcast(FromJid.GetFullPath(), Message->Body);
}

void UChat::Message(const FString& UserName, const FString& Recipient, const FString& Type, const FString& MessagePayload)
{
	if (XmppConnection->Messages().IsValid())
	{
		FXmppMessage Message;
		Message.FromJid.Id = UserName;
		Message.ToJid.Id = Recipient;
		Message.Type = Type;
		Message.Payload = MessagePayload;
		XmppConnection->Messages()->SendMessage(Recipient, Message);
	}
}

void UChat::PrivateChat(const FString& UserName, const FString& Recipient, const FString& Body)
{
	if (XmppConnection->PrivateChat().IsValid())
	{
		FXmppChatMessage ChatMessage;
		ChatMessage.FromJid.Id = UserName;
		ChatMessage.ToJid.Id = Recipient;
		ChatMessage.Body = Body;
		XmppConnection->PrivateChat()->SendChat(Recipient, ChatMessage);
	}
}


/***************** Presence **************************/

void UChat::Presence(bool bIsAvailable, EUXmppPresenceStatus::Type Status, const FString& StatusStr)
{
	if (XmppConnection->Presence().IsValid())
	{		
		FXmppUserPresence XmppPresence = XmppConnection->Presence()->GetPresence();
		XmppPresence.bIsAvailable = bIsAvailable;
		XmppPresence.Status = UChatUtil::GetEXmppPresenceStatus(Status);
		XmppConnection->Presence()->UpdatePresence(XmppPresence);
	}
}

void UChat::PresenceQuery(const FString& User)
{
	if (XmppConnection->Presence().IsValid())
	{
		XmppConnection->Presence()->QueryPresence(User);
	}
}

void UChat::PresenceGetRosterMembers(TArray<FString>& Members)
{
	if (XmppConnection->Presence().IsValid())
	{
		TArray<FXmppUserJid> MemberJids;
		XmppConnection->Presence()->GetRosterMembers(MemberJids);

		for (auto& Jid : MemberJids)
		{			
			Members.Push(Jid.Id);
		}
	}
}

// TODO:
// FXmppUserPresenceJingle and FXmppUserPresence
// TArray<TSharedPtr<FXmppUserPresence>> FXmppPresenceJingle::GetRosterPresence(const FString& UserId)
// virtual FOnXmppPresenceReceived& OnReceivePresence() override { return OnXmppPresenceReceivedDelegate; }
// Needed?
// 	virtual bool UpdatePresence(const FXmppUserPresence& Presence) override;
//	virtual const FXmppUserPresence& GetPresence() const override;

/***************** MUC **************************/

void UChat::OnMUCReceiveMessageFunc(const TSharedRef<IXmppConnection>& Connection, const FXmppRoomId& RoomId, const FXmppUserJid& UserJid, const TSharedRef<FXmppChatMessage>& ChatMsg)
{
	if (Connection->MultiUserChat().IsValid())
	{
		OnMUCReceiveMessage.Broadcast(static_cast<FString>(RoomId), UserJid.Resource, *ChatMsg->Body);
	}
}

void UChat::OnMUCRoomJoinPublicCompleteFunc(const TSharedRef<IXmppConnection>& Connection, bool bSuccess, const FXmppRoomId& RoomId, const FString& Error)
{
	OnMUCRoomJoinPublicComplete.Broadcast(bSuccess, static_cast<FString>(RoomId), Error);
}

void UChat::OnMUCRoomJoinPrivateCompleteFunc(const TSharedRef<IXmppConnection>& Connection, bool bSuccess, const FXmppRoomId& RoomId, const FString& Error)
{
	OnMUCRoomJoinPrivateComplete.Broadcast(bSuccess, static_cast<FString>(RoomId), Error);
}

void UChat::OnMUCRoomMemberJoinFunc(const TSharedRef<IXmppConnection>& Connection, const FXmppRoomId& RoomId, const FXmppUserJid& UserJid)
{	
	UE_LOG(LogChat, Log, TEXT("UChat::OnMUCRoomMemberJoin RoomId=%s UserJid=%s"), *static_cast<FString>(RoomId), *UserJid.GetFullPath());
	OnMUCRoomMemberJoin.Broadcast(static_cast<FString>(RoomId), UserJid.Resource);
}

void UChat::OnMUCRoomMemberExitFunc(const TSharedRef<IXmppConnection>& Connection, const FXmppRoomId& RoomId, const FXmppUserJid& UserJid)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnMUCRoomMemberExit RoomId=%s UserJid=%s"), *static_cast<FString>(RoomId), *UserJid.GetFullPath());
	OnMUCRoomMemberExit.Broadcast(static_cast<FString>(RoomId), UserJid.Resource);
}

void UChat::OnMUCRoomMemberChangedFunc(const TSharedRef<IXmppConnection>& Connection, const FXmppRoomId& RoomId, const FXmppUserJid& UserJid)
{
	UE_LOG(LogChat, Log, TEXT("UChat::OnMUCRoomMemberChanged RoomId=%s UserJid=%s"), *static_cast<FString>(RoomId), *UserJid.GetFullPath());
	OnMUCRoomMemberChanged.Broadcast(static_cast<FString>(RoomId), UserJid.Resource);
}

void UChat::MucCreate(const FString& UserName, const FString& RoomId, bool bIsPrivate, const FString& Password)
{
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		FXmppRoomConfig RoomConfig;
		RoomConfig.RoomName = RoomId;		
		RoomConfig.bIsPersistent = false;
		RoomConfig.bIsPrivate = bIsPrivate;		
		RoomConfig.Password = Password;
		XmppConnection->MultiUserChat()->CreateRoom(RoomId, UserName, RoomConfig);
	}
}

void UChat::MucJoin(const FString& RoomId, const FString& Nickname, const FString& Password)
{
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		if (Password.IsEmpty())
		{
			XmppConnection->MultiUserChat()->JoinPublicRoom(RoomId, Nickname);
		}
		else
		{
			XmppConnection->MultiUserChat()->JoinPrivateRoom(RoomId, Nickname, Password);
		}
	}
}

void UChat::MucExit(const FString& RoomId)
{
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		XmppConnection->MultiUserChat()->ExitRoom(RoomId);
	}
}

void UChat::MucChat(const FString& RoomId, const FString& Body, const FString& ChatInfo)
{				
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		XmppConnection->MultiUserChat()->SendChat(RoomId, Body, ChatInfo);
	}
}

void UChat::MucConfig(const FString& UserName, const FString& RoomId, bool bIsPrivate, const FString& Password)
{
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		FXmppRoomConfig RoomConfig;
		RoomConfig.bIsPrivate = bIsPrivate;
		RoomConfig.Password = Password;
		XmppConnection->MultiUserChat()->ConfigureRoom(RoomId, RoomConfig);
	}
}

void UChat::MucRefresh(const FString& RoomId)
{
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		XmppConnection->MultiUserChat()->RefreshRoomInfo(RoomId);
	}
}

void UChat::MucGetMembers(const FString& RoomId, TArray<UChatMember*>& Members)
{
	if (XmppConnection.IsValid() && XmppConnection->MultiUserChat().IsValid())
	{
		TArray< FXmppChatMemberRef > OutMembers;
		XmppConnection->MultiUserChat()->GetMembers(RoomId, OutMembers);

		Members.Empty();
		Members.Reserve(OutMembers.Num());
		for (auto& Member : OutMembers)
		{
			UChatMember* UMember = NewObject<UChatMember>();
			UMember->ConvertFrom(Member.Get());
			Members.Add(UMember);
		}
	}
}

/***************** PubSub **************************/

void UChat::PubSubCreate(const FString& NodeId)
{
	if (XmppConnection.IsValid() && XmppConnection->PubSub().IsValid())
	{
		FXmppPubSubConfig PubSubConfig;
		XmppConnection->PubSub()->CreateNode(NodeId, PubSubConfig);
	}
}

void UChat::PubSubDestroy(const FString& NodeId)
{
	if (XmppConnection.IsValid() && XmppConnection->PubSub().IsValid())
	{
		XmppConnection->PubSub()->DestroyNode(NodeId);
	}
}

void UChat::PubSubSubscribe(const FString& NodeId)
{
	if (XmppConnection.IsValid() && XmppConnection->PubSub().IsValid())
	{
		XmppConnection->PubSub()->Subscribe(NodeId);
	}
}

void UChat::PubSubUnsubscribe(const FString& NodeId)
{
	if (XmppConnection.IsValid() && XmppConnection->PubSub().IsValid())
	{
		XmppConnection->PubSub()->Unsubscribe(NodeId);
	}
}

void UChat::PubSubPublish(const FString& NodeId, const FString& Payload)
{
	if (XmppConnection.IsValid() && XmppConnection->PubSub().IsValid())
	{
		FXmppPubSubMessage Message;
		Message.Payload = Payload;
		XmppConnection->PubSub()->PublishMessage(NodeId, Message);
	}
}



