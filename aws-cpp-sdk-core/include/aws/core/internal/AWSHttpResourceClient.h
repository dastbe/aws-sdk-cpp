/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  *
  *  http://aws.amazon.com/apache2.0
  *
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */

#pragma once

#include <aws/core/Core_EXPORTS.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/client/AWSErrorMarshaller.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/utils/DateTime.h>
#include <memory>

namespace Aws
{
    namespace Http
    {
        class HttpClient;
    } // namespace Http

    namespace Internal
    {
        /**
         * Simple client for accessing the AWS remote data by Http.
         * Currently we use it to access EC2 Metadata and ECS Credential.
         */
        class AWS_CORE_API AWSHttpResourceClient
        {
        public:
            /**
             * Builds an AWSHttpResourceClient instance by using default http stack.
             */
            AWSHttpResourceClient(const char* logtag = "AWSHttpResourceClient");
            AWSHttpResourceClient(const Client::ClientConfiguration& clientConfiguration, const char* logtag = "AWSHttpResourceClient");

            AWSHttpResourceClient& operator =(const AWSHttpResourceClient& rhs) = delete;
            AWSHttpResourceClient(const AWSHttpResourceClient& rhs) = delete;
            AWSHttpResourceClient& operator =(const AWSHttpResourceClient&& rhs) = delete;
            AWSHttpResourceClient(const AWSHttpResourceClient&& rhs) = delete;

            virtual ~AWSHttpResourceClient();

            /**
             * Connects to an HTTP endpoint to read the specified resource and returns the text contents.
             * The resource URI = endpoint + resourcePath (e.g:http://domain/path/to/res)
             * @param endpoint The HTTP resource to connect to.
             * @param resourcePath A path appended to the endpoint to form the final URI.
             * @param authToken An optional authorization token that will be passed as the value of the HTTP
             * header 'Authorization'.
             * @return The response from the HTTP endpoint as a string.
             */
            virtual Aws::String GetResource(const char* endpoint, const char* resourcePath, const char* authToken) const;

            /**
             * Set an error marshaller so as to marshall error type from http response body if any.
             * So that it can work with retry strategy to decide if a request should retry or not.
             */
            void SetErrorMarshaller(Aws::UniquePtr<Client::AWSErrorMarshaller> errorMarshaller) { m_errorMarshaller = std::move(errorMarshaller); }
            const Client::AWSErrorMarshaller* GetErrorMarshaller() const { return m_errorMarshaller.get(); }

        protected:
            Aws::String m_logtag;

        private:
            std::shared_ptr<Client::RetryStrategy> m_retryStrategy;
            std::shared_ptr<Http::HttpClient> m_httpClient;
            Aws::UniquePtr<Client::AWSErrorMarshaller> m_errorMarshaller;
        };

        /**
         * Derived class to support retrieving of EC2 Metadata
         */
        class AWS_CORE_API EC2MetadataClient : public AWSHttpResourceClient
        {
        public:
            /**
             * Build an instance with default EC2 Metadata service endpoint
             */
            EC2MetadataClient(const char* endpoint = "http://169.254.169.254");
            EC2MetadataClient(const Client::ClientConfiguration& clientConfiguration, const char* endpoint = "http://169.254.169.254");

            EC2MetadataClient& operator =(const EC2MetadataClient& rhs) = delete;
            EC2MetadataClient(const EC2MetadataClient& rhs) = delete;
            EC2MetadataClient& operator =(const EC2MetadataClient&& rhs) = delete;
            EC2MetadataClient(const EC2MetadataClient&& rhs) = delete;

            virtual ~EC2MetadataClient();

            using AWSHttpResourceClient::GetResource;

            /**
            * Connects to the metadata service to read the specified resource and
            * returns the text contents. The resource URI = m_endpoint + resourcePath.
            */
            virtual Aws::String GetResource(const char* resourcePath) const;

            /**
             * Connects to the Amazon EC2 Instance Metadata Service to retrieve the
             * default credential information (if any).
             */
            virtual Aws::String GetDefaultCredentials() const;

            /**
             * connects to the Amazon EC2 Instance metadata Service to retrieve the region
             * the current EC2 instance is running in.
             */
            virtual Aws::String GetCurrentRegion() const;

        private:
            Aws::String m_endpoint;
        };

        /**
         * Derived class to support retrieving of ECS Credentials
         */
        class AWS_CORE_API ECSCredentialsClient : public AWSHttpResourceClient
        {
        public:
            /**
             * Build an instance with default ECS service endpoint
             * @param resourcePath The path part of the metadata URL
             * @param endpoint The URL authority to hit. Default is the IP address of the Task metadata service endpoint.
             */
            ECSCredentialsClient(const char* resourcePath, const char* endpoint = "http://169.254.170.2",
                    const char* authToken = "");
            ECSCredentialsClient(const Client::ClientConfiguration& clientConfiguration,
                                 const char* resourcePath, const char* endpoint = "http://169.254.170.2",
                                 const char* authToken = "");

            ECSCredentialsClient& operator =(ECSCredentialsClient& rhs) = delete;
            ECSCredentialsClient(const ECSCredentialsClient& rhs) = delete;
            ECSCredentialsClient& operator =(ECSCredentialsClient&& rhs) = delete;
            ECSCredentialsClient(const ECSCredentialsClient&& rhs) = delete;

            /**
             * Connects to the Amazon ECS service to retrieve the credential
             */
            virtual Aws::String GetECSCredentials() const
            {
                return GetResource(m_endpoint.c_str(), m_resourcePath.c_str(), m_token.c_str());
            }

        private:
            Aws::String m_resourcePath;
            Aws::String m_endpoint;
            Aws::String m_token;
        };

        /**
         * To support retrieving credentials from STS.
         * Note that STS accepts request with protocol of queryxml. Calling GetResource() will trigger
         * a query request using AWSHttpResourceClient under the hood.
         */
        class AWS_CORE_API STSCredentialsClient : public AWSHttpResourceClient
        {
        public:
            /**
             * Initializes the provider to retrieve credentials from STS when it expires.
             */
            STSCredentialsClient(const Client::ClientConfiguration& clientConfiguration);

            STSCredentialsClient& operator =(STSCredentialsClient& rhs) = delete;
            STSCredentialsClient(const STSCredentialsClient& rhs) = delete;
            STSCredentialsClient& operator =(STSCredentialsClient&& rhs) = delete;
            STSCredentialsClient(const STSCredentialsClient&& rhs) = delete;

            // If you want to make an AssumeRoleWithWebIdentity call to sts. use these classes to pass data to and get info from STSCredentialsClient client.
            // If you want to make an AssumeRole call to sts, define the request/result members class/struct like this.
            struct STSAssumeRoleWithWebIdentityRequest
            {
                Aws::String roleSessionName;
                Aws::String roleArn;
                Aws::String webIdentityToken;
            };

            struct STSAssumeRoleWithWebIdentityResult
            {
                Aws::Auth::AWSCredentials creds;
                Aws::Utils::DateTime expiration;
            };

            STSAssumeRoleWithWebIdentityResult GetAssumeRoleWithWebIdentityCredentials(const STSAssumeRoleWithWebIdentityRequest& request);

        private:
            Aws::String m_endpoint;
        };
    } // namespace Internal
} // namespace Aws
