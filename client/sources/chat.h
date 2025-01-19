#pragma once
#include<vector>
#include"request.h"

enum State {
	NOT_STATED,
	ALLOWED,
	FORBIDDEN,
	AUTHORIZED,
	NOT_AUTHORIZED
};

class Chat {
public:
	State getState() const { return isAllowed; };
	void setState(State value) { isAllowed = value; };

	req::ReceiverData getReceiver() const { return m_receiver_data; };
	void setReceiver(std::string receiverLogin) { m_receiver_data = receiverLogin; };

	std::vector<std::string>& getSendMsgVec() { return m_vec_send_messages; };
	std::vector<std::string>& getReceivedMsgVec() { return m_vec_received_messages; };

	void setLastIncomeMsg(const std::string& msg) { m_last_incoming_msg = msg; };
	std::string& getLastIncomeMsg() { return m_last_incoming_msg; };

private:
	std::string m_last_incoming_msg;
	std::vector<std::string> m_vec_send_messages;
	std::vector<std::string> m_vec_received_messages;
	req::ReceiverData m_receiver_data;
	State isAllowed = NOT_STATED;
};