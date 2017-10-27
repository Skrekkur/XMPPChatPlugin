# XMPPChatPlugin
Plugin for Unreal Engine 4 (UE4) to use XMPP chat from Blueprint

Since the plugin is mostly a BP interface to the existing Xmpp chat code from Epic (which itself is built upon libjingle from the webrtc project), I suggest looking at https://github.com/DescendentStudios/XMPPChatPlugin/blob/master/Source/XMPPChat/Public/Chat.h and the libjingle/webrtc docs for additional info.

In general, create the Chat object and set the delegate handlers on it for common operations, including OnChatLoginComplete, OnChatReceiveMessage, OnPrivateChatReceiveMessage, OnMUCReceiveMessage, OnMUCRoomJoinPublicComplete, etc. Then call Login() with XMPP user credentials and server info, use MucChat(), Message(), and PrivateMessage() to send messages, handle responses from the delegates, and call Logout() at the end.
